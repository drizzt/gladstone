#!/bin/bash

cd $1

for i in *
do
  j=`echo $i | tr '[A-Z]' '[a-z]'`
  mv $i $j
done
