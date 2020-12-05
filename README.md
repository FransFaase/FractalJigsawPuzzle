# FractalJigsawPuzzle
C++ program for generating SVG files representing Fractal Jigsaw Puzzles optimized for laser cutting

For a description about how to use this program see:
https://tkkrlab.nl/wiki/Fractal_Jigsaw_Puzzle

For blogs about the developement see:
http://www.iwriteiam.nl/Dpuzzle.html#fracjig

Ready made puzzles can be ordered from:
https://www.annabelester.nl/

# Example usage

Start for example with:
```
./pianofrac gen_ec -con -range=2-3 -with_name | ./ExactCover | ./pianofrac normalize -minimal >sols.txt
```
The file `sols.txt` will now contains all solutions. Some solutions will use the same pieces and thus
can be viewed as solutions for the same puzzle. To get the file with all puzzles, use:
```
./pianofrac used_pieces <sols.txt | sort | uniq -c | sort >puzzles.txt
```
Now you can select one of the puzzles. Take for example: `1,1,1,2,2,2,2,2,3,4,4,4,4,7,7,10`.
To print all the solutions for this puzzle, use the command:
```
./pianofrac filter 1,1,1,2,2,2,2,2,3,4,4,4,4,7,7,10 <sols.txt | ./pianofrac print
```
To generate an SVG file for this puzzle, use, for example:
```
./pianofrac filter 1,1,1,2,2,2,2,2,3,4,4,4,4,7,7,10 <sols.txt | ./pianofrac svg -space=2 >puzzle.svg
```
