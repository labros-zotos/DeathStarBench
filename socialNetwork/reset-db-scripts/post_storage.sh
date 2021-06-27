#!/bin/bash

mongo post-storage-mongodb:27017/post-storage --eval "var colName='post-storage'"  truncate_collection.js
echo flush_all > /dev/tcp/post-storage-memcached/11211