# LynxBot: A bot for Old School Runescape Twitch chat

## Contents

0. [Introduction](#introduction)
1. [Setup](#Setup)
2. [Additional Information](#additional-information)

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

You can now launch the bot and it will join your channel, ready for use.

For more detailed setup instructions, see the
[wiki](https://github.com/frolv/lynxbot/wiki/Setup).

## Additional Information

See the [wiki](https://github.com/frolv/lynxbot/wiki) for full documentation
on LynxBot's functionality.
