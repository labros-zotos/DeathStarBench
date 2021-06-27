#!/bin/bash

mongo social-graph-mongodb:27017/social-graph --eval "var colName='social-graph'"  truncate_collection.js
redis-cli -h social-graph-redis -p 6379 flushdb