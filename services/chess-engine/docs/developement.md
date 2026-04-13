In this document I’ll be discussing my process for building
a chess engine from end to end. Here I’ll be dialing into the design of the
system, not only from the product perspective but from an engineering perspective.
What are the tools that I’m using, what levels of abstraction at I working it?
How can I measure these levels of abstraction? What skills am I building? What skills
am I utilizing? What skills best transfer to industry? And for what types of
roles? What’s difficult for me? What’s easy for me? How difficult would this be
for others? How can I exploit the tools and knowledge I already have and claim
to know to their fullest extent?

I’m going to be documenting my thinking process for start to end.

Suppose I am starting a new project (in this case a chess
engine)

What exactly is meant by “chess engine” can be pretty ambiguous and may vary greatly
depending on who you ask, so I’m going to define in detail what “chess engine”
means for this project and what exactly I want to accomplish (as of right now).

Def 1. Chess engine

A chess engine is a computer program that analyzes chess positions
and makes the best move.

Theorem 1. A chess engine has analyzed a single position if it has used an
evaluation function to generate a score for the position.

Therem 2. A chess engine has analyzed multiple positions if given a starting
position, and a search depth,  and the
available moves; for each available move, it evaluates a sequence of positions until
the search depth is reached, generating an evaluation score for each available
move from the starting position

Theorem 3. The engine has made this best move if the
sequences of moves following that move up to a given depth leads to best score considering
all other possible moves and their sequences.

How do I want the system to work from a user perspective?

Simple.

1. User runs the program, choosing a color (white
   or black) and depth (how far the engine should search). If a depth is not
   selected, a default depth is preset.
2. A board is shown in the terminal. The user types
   their move on the command line, ex: “e2e4 h5h6”.
3. The move is shown on the board
4. The engine accepts the move and then makes its
   next move.
5. This process repeats until a terminal state is
   reached or the user ends the game.

How  do we want the
system to work from an engineering perspective?

The system shall be divided into core Modules:

- Position
  (owns and modifies the game state)
- Search
  (how the engine “thinks”) – takes a position and a depth as input, returns best
  move and score as output
- MoveGeneration
  (takes in a position is input) returns available moves for given side as output
  (stateless class)
- Rules (stateless class) game and move legalities
  (stateless helper service used by Movegenration)
- Evaluation (stateless helper service used by
  search)

We’ll split up the development of this project into iterations
or checkpoints listed as

“V1, V2, V3, …..”

These checkpoints are not fixed, as we’ll progress in development, we will have
to modify. Refractor, redesign. However, good planning and design principles
will aid in minimizing this as much as possible along with the complexity of expanding
the system.

We will evaluate success of each checkpoint by clearly defining
goals, expectations, and tests that must be passed

V1: Move generation & Rules (no search yet)

- The system shall implement modules just enough such
  that Move generation and rules successfully pass tests

Modules to be worked on:

- Position
- MoveGen
- Rules

Tests to be verified:

TestMoves: test that all piece moves are implemented
correctly along with edge cases (pawn, bishop, knight, etc)

- Test Rules:

oTest enpassent

oTets checks

oEdge cases and general chess rules

(Manually implementing all of these
would suck, definitely utilize an LLM)

V2: Search & Eval

Modules to be worked on:

- Search
- Eval

May also need to add to:

- Position

Minimum working requirements:

Expectations:

- Given a position, the engine searched up to a
  given depth and returns the best move
- It does this within a reasonable amount of time
  (not a priority requirement, still pretty vague

V3:
