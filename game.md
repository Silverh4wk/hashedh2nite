- 2 teams: (draft name): Agents and Spies
- A random english word is chosen
- At the start of the game each player's name is generated based on 1 letter from the random word
    - Example: 
        - Word: Cat
        - Player names (3):
            - L(a)rry
            - (C)arl
            - E(t)han
- Game loop:
    - Discussion phase and then a player has to "propose" a node.
    - All players vote on the proposed node.
    - If the node goes through the players in the node are shown a letter from their name*.
    - After each node the players have to vote a letter.
    - If 3* incorrect letters (not in the word) are voted in the spies win.
    - Agents win by voting in all the letters of the word.


    *The mechanics of what letters are shown need to be ironed out for these reasons:
    - Spies should try to misguide the agents.
    - If the same letter is shown to all people in the node then figuring out a spy is trivial.
    - If different letters are shown to each player in the node then what letter? 
    ** this is unimportant to think about at the current stage **
