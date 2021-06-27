#!/bin/bash

mongo url-shorten-mongodb:27017/url-shorten --eval "var colName='url-shorten'"  truncate_collection.js
echo flush_all > /dev/tcp/url-shorten-memcached/11211