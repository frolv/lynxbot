# LynxBot: A bot for Old School Runescape Twitch chat

## Contents

0. [Introduction](#introduction)
1. [Setup](#Setup)
2. [Default Commands](#default-commands)
3. [Custom Commands](#custom-commands)
4. [Automated Responses](#automated-responses)

## Introduction

LynxBot is a Twitch.tv IRC bot designed specifically for use on Old School
Runescape streams. It offers a number of commands that assist Runescape
streams, such as Grand Exchange and Hiscore lookups, as well as the flexibility
of customization through user defined commands, automated responses and more.

## Setup

In order to use this bot, you will have to create a new account on
[Twitch](http://twitch.tv) on which to run it.

After creating the account for your bot, visit
[twitchapps](https://twitchapps.com/tmi) to generate an **oauth token** for it.
This is the password your bot will use to login to Twitch IRC.

Open up [settings.txt](/settings.txt), and enter the name of your bot's Twitch
account, the channel you want to join (typically your Twitch username prepended
with a `#`) and the oauth token you generated (including the `oauth:`) in their
respective fields.

## Default Commands

See [Default Commands](https://github.com/frolv/lynxbot/wiki/Default-Commands) in the wiki.

## Custom Commands

LynxBot also supports user-defined commands which print a string of text when
called.  Custom commands can be added or modified using the following three
commands:

* **ADDCOM**

  Syntax: ``` $addcom [-c COOLDOWN] COMMAND RESPONSE ```

  `COMMAND` is the keyword which activates the command. It is given without the
  leading dollar sign. The name cannot be the same as any of the default
  commands or any previously added user commands and has a limit of 20
  characters.

  `RESPONSE` is the string to print when the command is called.

  If the `-c` flag is provided, `COOLDOWN` is a number specifying the cooldown
  (in seconds) of the command. By default, all custom commands have a 15 second
  cooldown.

  Examples:

  0. Without cooldown flag ``` $addcom hi Hello, World!  ```

    This adds a command called `hi` with a 15 second cooldown and the response
    `Hello, World!`. To activate this command, a user would type `$hi` in the
    chat. Note that the dollar sign is implicitly added - do not specify it in
    the `addcom` call.

  1. With cooldown flag ``` $addcom -c 60 cd This command has a 60s cooldown
  ```

    This adds a command named `cd` with a 60 second cooldown. It is activated
    by typing `$cd` in the chat and the bot responds with `This command has a
    60s cooldown`.

* **EDITCOM**

  Syntax: ``` $editcom [-c COOLDOWN] COMMAND [RESPONSE] ```

  All variables are the same as in `$addcom`, except that `COMMAND` must refer
  to an existing command added using `$addcom`.  At least one of `COOLDOWN` or
  `RESPONSE` must be provided.

  The `$editcom` command edits the cooldown or the response of an existing
  custom command.

  Examples: For the examples below, assume a custom command `hi` exists with a
  15s cooldown.

  0. Editing cooldown only ``` $editcom -c 60 hi ```

    This changes the cooldown of `hi` to 60 seconds.

  1. Editing response only ``` $editcom hi hey wassup Hello ```

    This changes the response of the `hi` command to `hey wassup hello`.

  2. Editing both cooldown and response ``` $editcom -c 60 hi hey wassup hello
  ```

    This combines the actions of parts i and ii in a single statement.

* **DELCOM**

  Syntax: ``` $delcom COMMAND ```

  `COMMAND` is a previously added custom command.

  The `$delcom` command permanently deletes the given custom command.

## Automated Responses

Unlike a command, which is specifically called by a user, an automated response
is sent by the bot whenever it reads a certain string or pattern within a chat
message.
