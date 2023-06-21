#!/bin/bash

SOURCE_FILE="./data/dataset.txt";
COMPRESSED_FILE="./data/dataset_compressed";
NATIVETHREADS="./Native_Threads/app";
FFHUFFMAN="./fastflow/ffhuffman";
SEQHUFFMAN="Seq/Seqhuffman";

echo "Deleting old Compressed Files...";

for((i=1; i<=64; i*=2)) ; do rm $COMPRESSED_FILE"_NT_"$i".txt"; done
for((i=1; i<=64; i*=2)) ; do rm $COMPRESSED_FILE"_FF_"$i".txt"; done

echo "Source File is:" $SOURCE_FILE;
echo "Starting SEQ Huffman";

echo "Starting Native Threads Implementation:" $NATIVETHREADS;

for((i=1; i<=64; i*=2)) ; do $NATIVETHREADS $SOURCE_FILE $COMPRESSED_FILE"_NT_"$i".txt" $i; done


echo "Starting FastFlow Implementation:" $FFHUFFMAN;

for((i=1; i<=64; i*=2)) ; do $FFHUFFMAN $SOURCE_FILE $COMPRESSED_FILE"_FF_"$i".txt" $i; done


