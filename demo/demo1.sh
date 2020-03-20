#!/bin/bash

cd /workspace/apps/adpredictor
python3 /workspace/metaprograms/udt/udt_openmp.py
cd openmp_project ; make ; make run


