#!/bin/bash

mongo user-mongodb:27017/user --eval "var colName='user'"  truncate_collection.js
echo flush_all > /dev/tcp/user-memcached/11211