#!/bin/bash

echo -e '\033[0;36mCleaning..\033[0m'

make clean >> /dev/null
rm -rf iso influx.iso