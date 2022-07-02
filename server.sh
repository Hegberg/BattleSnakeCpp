#!/bin/bash
until ./main; do
    echo "Server 'main' exited with code $?.  Respawning.." >&2
done