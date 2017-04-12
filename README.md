# RandomGen
Random dungeon generator

A game I am currently working on with a friend requires randomly generated stealth levels with randomly placed enemies, treasures and power-ups.
I decided to write a separate generator using C++/SFML, then port it to C#/Unity.

Also includes ability visualize generator through the magic of \~\~MuLtIThReAdIng\~\~.

Note: ~~I'll add buttons after I'm done, until then bare with keyboard controls~~ I'll add buttons later

## Directions
 * R - generate new map
 * G - toggle maze generator(Recursive is default)
 * T - toggles visualization(disabled by default)

## Build Requirements:
SFML 2.4.2 for rendering

## References:
* Dungeon Generation
  * Bob Nystrom, "Rooms and Mazes : A Procedural Dungeon Generator" - [stuffwithstuff.com](http://journal.stuffwithstuff.com/2014/12/21/rooms-and-mazes/)
* Maze Generation
  * Jamis Buck, ""Algorithm" is not a !@%#$@ 4-letter Word" - [jamisbuck.org](http://www.jamisbuck.org/presentations/rubyconf2011/)
  * Jamis Buck, "Maze Generation: Recursive Backtracking" - [jamisbuck.org](http://weblog.jamisbuck.org/2010/12/27/maze-generation-recursive-backtracking)
  * Jamis Buck, "Maze Generation: Eller's Algorithm" - [jamisbuck.org](http://weblog.jamisbuck.org/2010/12/29/maze-generation-eller-s-algorithm.html)
* Random Numbers
  * Dr Mads Haahr, "Introduction to Randomness and Random Numbers" - [random.org](https://www.random.org/randomness/)
