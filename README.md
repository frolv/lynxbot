# BOTNAME: A bot for Old School Runescape Twitch chat

## Contents

0. [Introduction](#introduction)
1. [Installation](#installation)
2. [Default Commands](#default-commands)
3. [Custom Commands](#custom-commands)
4. [Automated Responses](#automated-responses)

## Introduction

BOTNAME is a Twitch.tv IRC bot designed specifically for use on Old School Runescape streams. It offers a number of commands that assist

## Installation

Some text here.

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

* **8BALL**

  Syntax:
  ```
  $8ball QUESTION
  ```

  `QUESTION` is a sentence ending with a question mark (?).

  The `8ball` command responds to the question with one of the typical Magic 8 Ball responses.

* **ADDCOM, DELCOM and EDITCOM**

  See [Custom Commands](#custom-commands).

## Custom Commands

BOTNAME also supports user-defined commands which print a string of text when called.
Custom commands can be added or modified using the following three commands:

* **ADDCOM**

  Syntax:
  ```
  $addcom [-c COOLDOWN] COMMAND RESPONSE
  ```

  `COMMAND` is the keyword which activates the command. It is given without the leading dollar sign. The name cannot be the same as any of the default commands or any previously added user commands and has a limit of 20 characters.

  `RESPONSE` is the string to print when the command is called.

  If the `-c` flag is provided, `COOLDOWN` is a number specifying the cooldown (in seconds) of the command. By default, all custom commands have a 15 second cooldown.

  Examples:

  0. Without cooldown flag
    ```
    $addcom hi Hello, World!
    ```

    This adds a command called `hi` with a 15 second cooldown and the response `Hello, World!`. To activate this command, a user would type `$hi` in the chat. Note that the dollar sign is implicitly added - do not specify it in the `addcom` call.

  1. With cooldown flag
    ```
    $addcom -c 60 cd This command has a 60s cooldown
    ```

    This adds a command named `cd` with a 60 second cooldown. It is activated by typing `$cd` in the chat and the bot responds with `This command has a 60s cooldown`.

## Automated Responses

Unlike a command, which is specifically called by a user, an automated response is sent by the bot whenever it reads a certain string or pattern within a chat message.
