# BOTNAME: A bot for Old School Runescape Twitch chat

---

## Contents

0. [Introduction](#Introduction)
1. [Installation](#Installation)
2. [Default Commands](#Default-Commands)
3. [Custom Commands](#Custom-Commands)
4. [Automated Responses](#Automated-Responses)

---

## Introduction

BOTNAME is a Twitch.tv IRC bot designed specifically for use on Old School Runescape streams. It offers a number of commands that assist

---

## Installation

Some text here.

---

## Default Commands

All BOTNAME commands begin with a dollar sign `$` and take zero or more arguments.
In the examples below, all arguments passed to commands are written in `CAPS`.
Optional arguments are surrounded by square brackets.

* **EHP**

  Syntax:
  ```
  $ehp [NAME]
  ```

  `NAME` represents the in-game name of an Old School Runescape player. If it is specified, the `$ehp` command will look up that player on Crystal Math Labs and print their total EHP, EHP rank and EHP gained in the last week.

  If `NAME` is not specified, the command will simply print a short explanation of EHP, with a link to the Crystal Math Labs supplies calc page.

* **LEVEL**

  Syntax:
  ```
  $(level|lvl) SKILL NAME
  ```

  `SKILL` represents one of the 24 skills in Old School Runescape, including total level. Shorthand forms for skills can be used (e.g. runecrafting -> rc).

  `NAME` is the in-game name of an Old School Runescape player.

  The `$lvl` command looks up the specified player on the Old School Runescape hiscores and prints their level, rank and experience in the given skill.

* **GE**

  Syntax:
  ```
  $ge ITEM
  ```

  `ITEM` represents the name of a tradeable item in Old School Runescape. If the item name contains multiple words, either underscores or spaces can be used as separators. For some of the more common items, shorthand names can be used (e.g. Saradomin godsword -> sgs). All possible items are specified within [itemids.json](/itemids.json).

  The `$ge` command looks up the specified item on the OSBuddy exchange and prints its current price.

* **CALC**

  Syntax:
  ```
  $calc EXPRESSION
  ```

  `EXPRESSION` is a mathematical expression containing a combination of numbers, parentheses and the operators + - * /.

  The `$calc` command evaluates the given expression and prints the result.

---

## Custom Commands

BOTNAME also supports user-defined commands which print a string of text when called.

---

## Automated Responses

Unlike a command, which is specifically called by a user, an automated response is sent by the bot whenever it reads a certain string or pattern within a chat message.
