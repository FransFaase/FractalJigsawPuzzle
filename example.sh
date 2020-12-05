#!/bin/bash
./pianofrac gen_ec -con -range=2-3 -with_name | ./ExactCover | ./pianofrac normalize -minimal >sols.txt
echo x
./pianofrac used_pieces <sols.txt | sort | uniq -c | sort >puzzles.txt
echo b
./pianofrac filter '1,1,1,2,2,2,2,2,3,4,4,4,4,7,7,10' <sols.txt | ./pianofrac print
echo c
./pianofrac filter '1,1,1,2,2,2,2,2,3,4,4,4,4,7,7,10' <sols.txt | ./pianofrac svg -space=2 >puzzle.svg
echo d
