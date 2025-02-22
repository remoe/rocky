/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#include "Runtime.h"
#include "Utils.h"
#include <vsg/app/Viewer.h>
#include <vsg/text/Font.h>
#include <vsg/io/read.h>
#include <shared_mutex>

using namespace ROCKY_NAMESPACE;

namespace
{
    /**
    * An update operation that maintains a priroity queue for update tasks.
    * This sits in the VSG viewer's update operations queue indefinitely
    * and runs once per frame. It chooses the highest priority task in its
    * queue and runs it. It will run one task per frame so that we do not
    * risk frame drops. It will automatically discard any tasks that have
    * been abandoned (no Future exists).
    */
    struct PriorityUpdateQueue : public vsg::Inherit<vsg::Operation, PriorityUpdateQueue>
    {
        std::mutex _mutex;

        struct Task {
            vsg::ref_ptr<vsg::Operation> function;
            std::function<float()> get_priority;
        };
        std::vector<Task> _queue;

        // runs one task per frame.
        void run() override
        {
            if (!_queue.empty())
            {
                Task task;
                {
                    std::scoped_lock lock(_mutex);

                    // sort from low to high priority
                    std::sort(_queue.begin(), _queue.end(),
                        [](const Task& lhs, const Task& rhs)
                        {
                            if (lhs.get_priority == nullptr)
                                return false;
                            else if (rhs.get_priority == nullptr)
                                return true;
                            else
                                return lhs.get_priority() < rhs.get_priority();
                        }
                    );

                    while (!_queue.empty())
                    {
                        // pop the highest priority off the back.
                        task = _queue.back();
                        _queue.pop_back();

                        // check for cancelation - if the task is already canceled, 
                        // discard it and fetch the next one.
                        auto po = dynamic_cast<Cancelable*>(task.function.get());
                        if (po == nullptr || !po->canceled())
                            break;
                        else
                            task = { };
                    }
                }

                if (task.function)
                {
                    task.function->run();
                }
            }
        }
    };

    //! Operation that removes a node from the scene graph.
    struct RemoveNodeAsync : public vsg::Inherit<vsg::Operation, RemoveNodeAsync>
    {
        vsg::observer_ptr<vsg::Group> _parent;
        unsigned _index;

        RemoveNodeAsync(vsg::Group* parent, unsigned index) :
            _parent(parent), _index(index) { }

        void run() override
        {
            vsg::ref_ptr<vsg::Group> parent = _parent;
            if (parent && parent->children.size() >= _index)
            {
                auto iter = parent->children.begin() + _index;
                parent->children.erase(iter);
            }
        }
    };

    struct SimpleUpdateOperation : public vsg::Inherit<vsg::Operation, SimpleUpdateOperation>
    {
        std::function<void()> _function;

        SimpleUpdateOperation(std::function<void()> function) :
            _function(function) { }

        void run() override
        {
            _function();
        };
    };
}


Runtime::Runtime()
{
    readerWriterOptions = vsg::Options::create();

    shaderCompileSettings = vsg::ShaderCompileSettings::create();

    _priorityUpdateQueue = PriorityUpdateQueue::create();

    const char* font_file = getenv("ROCKY_DEFAULT_FONT");
    if (font_file)
    {
        std::string filename(font_file);
        defaultFont = util::job::dispatch([this, filename](Cancelable&)
            {
                auto font = vsg::read_cast<vsg::Font>(filename, readerWriterOptions);
                if (!font)
                    Log::warn() << "Cannot load font \"" << filename << "\"" << std::endl;
                return font;
            });
    }
}

void
Runtime::runDuringUpdate(
    vsg::ref_ptr<vsg::Operation> function,
    std::function<float()> get_priority)
{
    auto pq = dynamic_cast<PriorityUpdateQueue*>(_priorityUpdateQueue.get());
    if (pq)
    {
        std::scoped_lock lock(pq->_mutex);

        if (pq->referenceCount() == 1)
        {
            viewer->updateOperations->add(_priorityUpdateQueue, vsg::UpdateOperations::ALL_FRAMES);
        }

        pq->_queue.push_back({ function, get_priority });
    }
}

void
Runtime::runDuringUpdate(std::function<void()> function)
{
    viewer->updateOperations->add(SimpleUpdateOperation::create(function));
}

void
Runtime::compile(vsg::ref_ptr<vsg::Object> compilable)
{
    auto cr = viewer->compileManager->compile(compilable);
    if (cr && cr.requiresViewerUpdate())
    {
        std::scoped_lock L(_compileResultsMutex);
        _compileResults.push_back(cr);
    }
}

void
Runtime::update()
{
    if (_compileResults.size() > 0)
    {
        std::scoped_lock L(_compileResultsMutex);
        Log::info() << "Updating viewer with " << _compileResults.size() << " results" << std::endl;
        for (auto& cr : _compileResults)
            vsg::updateViewer(*viewer, cr);
        _compileResults.clear();
    }
}

util::Future<bool>
Runtime::compileAndAddChild(vsg::ref_ptr<vsg::Group> parent, NodeFactory factory, const util::job& job_config)
{
    // This is a two-step procedure. First we have to create the child
    // by calling the Factory function, and compile it. These things happen 
    // in the asynchronous function. Secondly, we have to add the node to the
    // scene graph; this happens in VSG's update operations queue in some future
    // frame.

    // In order to return a future to the entire process, we will make our own
    // Promise and pass it along to both the async part and then on to the sync
    // update part. That way the user will be waiting on the final result of the
    // scene graph merge.

    util::Future<bool> promise;
    auto& runtime = *this;
    
    auto viewer = runtime.viewer;
    //auto compiler = runtime.compiler();
    //auto updates = runtime.updates();

    auto async_create_and_add_node = [viewer, promise, parent, factory](Cancelable& c) -> bool
    {
        if (c.canceled())
            return false;

        // create the child:
        auto child = factory(c);
        if (!child)
            return false;

        // compile the child:
        auto cr = viewer->compileManager->compile(child);

        // queue an update operation to add the child safely.
        // we pass along the original promise so these two operations appear as
        // one to the caller.
        auto add_child = [parent, child, viewer, cr](Cancelable& c) -> bool
        {
            if (c.canceled())
                return false;

            if (parent && child)
                parent->addChild(child);

            if (cr && cr.requiresViewerUpdate())
                vsg::updateViewer(*viewer, cr);

            return true;
        };        
        auto promise_op = util::PromiseOperation<bool>::create(promise, add_child);
        viewer->updateOperations->add(promise_op);

        return true;
    };

    auto future = util::job::dispatch(
        async_create_and_add_node,
        promise,
        job_config
    );

    return future;
}

void
Runtime::removeNode(vsg::Group* parent, unsigned index)
{
    auto remover = RemoveNodeAsync::create(parent, index);
    viewer->updateOperations->add(remover);
}
