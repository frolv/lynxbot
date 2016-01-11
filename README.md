# BOTNAME: A bot for Oldschool Runescape Twitch chat

## Contents

0. [Commands](#Commands)
1. 

## Commands

In the examples below, all arguments passed to commands are written in ```CAPS```.
Optional arguments are surrounded by square brackets.

* **EHP**

  Syntax:
  ```
  $ehp [NAME]
  ```
  
  If the ```NAME``` argument is not specified, the command prints a short explanation of EHP, with a link to CML's supplies calc.
  
  If ```NAME``` is specified, that user is looked up on CML and their total EHP, EHP rank and EHP last week are printed.
  
* **LEVEL**

  Syntax:
  ```
  $(level|lvl) SKILL NAME
  ```
  
  ```SKILL``` represents one of the 24 skills in Oldschool Runescape, including total level. Shorthand forms for skills can be used (e.g. runecrafting -> rc).
