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

See [Custom Commands](https://github.com/frolv/lynxbot/wiki/Custom-Commands) in the wiki.

## Automated Responses

Unlike a command, which is specifically called by a user, an automated response
is sent by the bot whenever it reads a certain string or pattern within a chat
message.
