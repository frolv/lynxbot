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

  Additional responses can be provided in the file [extra8ballresponses.txt](/extra8ballresponses.txt), one per line.

* **STRAWPOLL**

  Syntax:
  ```
  $(strawpoll|sp) [-flags] QUESTION | OPTION 1 | OPTION 2 | ...
  ```

  `QUESTION` and all `OPTION`s are separated by pipes `|`. At least two options must be provided.

  There are three different flags that can be used:
    * `b` - binary

      This creates a poll with yes and no as options.
      If this flag is specified, no options can be given in the command call.

    * `c` - captcha

      This flag creates a poll which requires the user to solve a CAPTCHA in order to vote.

    * `m` - multi

      This flag allows voters to select multiple poll options.

  The `$strawpoll` command creates a poll on [strawpoll](http://strawpoll.me) with the given question and options and posts a link to the poll in the chat.
  If no flags are specified, the poll defaults to a single choice per voter and no captcha.

  Examples:

  0. No flags

    ```
    $strawpoll What should I eat for dinner? | Saltine crackers | Bread | Lasagna
    ```

    This creates a poll with the question "What should I eat for dinner?" and the options "Saltine crackers", "Bread" and "Lasagna".
    [See here](http://strawpoll.me/6572349).

  1. Binary flag

    ```
    $strawpoll -b Should I do a 100m stake?
    ```

    This creates a poll with the question "Should I do a 100m stake?" and the options "yes" and "no".
    Notice how no answers are specified in the call.
    [See here](http://strawpoll.me/6572366).

  2. Multiple flags

    The following two calls are identical:
    ```
    $strawpoll -m -c Pick as many as you like | a | b | c | d
    ```
    ```
    $strawpoll -mc Pick as many as you like | a | b | c | d
    ```

    If multiple flags are provided they can either be separate or grouped.
    Both above calls will create a poll with the question "Pick as many as you like", and options "a", "b", "c" and "d".
    In this poll, voters can select multiple options (`m` flag) and must solve a captcha to vote (`c` flag).
    [See here](http://strawpoll.me/6572393).

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

* **EDITCOM**

  Syntax:
  ```
  $editcom [-c COOLDOWN] COMMAND [RESPONSE]
  ```

  All variables are the same as in `$addcom`, except that `COMMAND` must refer to an existing command added using `$addcom`.
  At least one of `COOLDOWN` or `RESPONSE` must be provided.

  The `$editcom` command edits the cooldown or the response of an existing custom command.

  Examples:
  For the examples below, assume a custom command `hi` exists with a 15s cooldown.

  0. Editing cooldown only
    ```
    $editcom -c 60 hi
    ```

    This changes the cooldown of `hi` to 60 seconds.

  1. Editing response only
    ```
    $editcom hi hey wassup Hello
    ```

    This changes the response of the `hi` command to `hey wassup hello`.

  2. Editing both cooldown and response
    ```
    $editcom -c 60 hi hey wassup hello
    ```

    This combines the actions of parts i and ii in a single statement.

* **DELCOM**

  Syntax:
  ```
  $delcom COMMAND
  ```

  `COMMAND` is a previously added custom command.

  The `$delcom` command permanently deletes the given custom command.

## Automated Responses

Unlike a command, which is specifically called by a user, an automated response is sent by the bot whenever it reads a certain string or pattern within a chat message.
