#!/usr/bin/env bash

bin=$HOME/bin
cfgdir=$HOME/.lynxbot

install() {
	if [[ ! -d $bin ]]; then
		mkdir -p $bin
		echo "created $bin"
	fi
	if [[ -z $(echo $PATH | grep -o $bin) ]]; then
		export PATH=${PATH}:$bin
		echo "added $bin to PATH for this shell"
	fi
	cp lynxbot $bin
	echo lynxbot copied to $bin
}

config() {
	[[ ! -d $cfgdir ]] && mkdir -p $cfgdir
	for f in *.txt; do
		n=$(echo $f | sed 's/\.txt//')
		if [[ ! -e $cfgdir/$n ]]; then
			cp $f $cfgdir/$n
		else
			echo $cfgdir/$n already exists: ignoring
		fi
	done
	[[ ! -d $cfgdir/json ]] && mkdir $cfgdir/json
	[[ ! -d $cfgdir/img ]] && mkdir $cfgdir/img
	for f in json/* img/*; do
		if [[ ! -e $cfgdir/$f ]]; then
			cp $f $cfgdir/$f
		else
			echo $cfgdir/$f already exists: ignoring
		fi
	done
	echo all configuration files copied to $cfgdir
}

if [[ ! -f lynxbot ]]; then
	echo "lynxbot not found. Run make first." >&2
else
	install
	config
	echo lynxbot successfully installed
fi
