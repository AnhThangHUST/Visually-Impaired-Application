#/bin/bash

search_dir="$PWD/image3"
for entry in "$search_dir"/*
do
  ./Preprocess $entry
done


