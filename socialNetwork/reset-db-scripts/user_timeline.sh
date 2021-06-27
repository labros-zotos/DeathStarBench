#!/bin/bash

mongo user-timeline-mongodb:27017/user-timeline --eval "var colName='user-timeline'"  truncate_collection.js
redis-cli -h user-timeline-redis -p 6379 flushdb