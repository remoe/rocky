/**
 * rocky c++
 * Copyright 2023 Pelican Mapping
 * MIT License
 */
#pragma once
#include <rocky/Common.h>
#include <rocky/Status.h>
#include <rocky/URI.h>
#include <rocky/TileKey.h>

namespace ROCKY_NAMESPACE
{
    class Image;

    namespace MBTiles
    {
        /**
        * Options for an MBTiles database
        */
        struct Options
        {
            optional<URI> uri;
            optional<std::string> format = "image/png";
            optional<bool> compress = false;
        };

        /**
         * Underlying driver for reading/writing an MBTiles database
         */
        class ROCKY_EXPORT Driver
        {
        public:
            Driver();
            ~Driver();

            Status open(
                const std::string& name,
                const Options& options,
                bool isWritingRequested,
                Profile& profile_inout,
                DataExtentList& dataExtents,
                const IOOptions& io);

            void close();

            Result<shared_ptr<Image>> read(
                const TileKey& key,
                const IOOptions& io) const;

            Status write(
                const TileKey& key,
                shared_ptr<Image> image,
                const IOOptions& io) const;

            void setDataExtents(const DataExtentList&);
            bool getMetaData(const std::string& name, std::string& value);
            bool putMetaData(const std::string& name, const std::string& value);

        private:
            void* _database;
            mutable unsigned _minLevel;
            mutable unsigned _maxLevel;
            shared_ptr<Image> _emptyImage;
            Options _options;
            std::string _tileFormat;
            bool _forceRGB;
            std::string _name;

            // because no one knows if/when sqlite3 is threadsafe.
            mutable std::mutex _mutex;

            bool createTables();
            void computeLevels();
            Result<int> readMaxLevel();
        };
    }
}
