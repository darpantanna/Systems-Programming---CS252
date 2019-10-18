#!/bin/bash

#DO NOT REMOVE THE FOLLOWING TWO LINES
git add $0 >> .local.git.out
git commit -a -m "Lab 2 commit" >> .local.git.out
git push >> .local.git.out || echo



#Your code here
#If <6 and >32 return

password=$( cat $1 )
#password=$1
P=${#password}
G=32
L=6
score=1

if [ $P -lt $L ]; then
	echo "Error: Password length invalid"
	exit
elif [ $P -gt $G ]; then
	echo "Error: Password length invalid"
	exit
fi

score=$P

#let score=score+1

if egrep -q [0-9] $1; then
	let score=score+5
fi

if egrep -q [#$+%@] $1; then
	let score=score+5
fi

if egrep -q [A-Za-z] $1; then
	let score=score+5
fi

if egrep -q '[0-9][0-9][0-9]' $1; then
	let score=score-3
fi

if egrep -q '[A-Z][A-Z][A-Z]' $1; then
	let score=score-3
fi

if egrep -q '[a-z][a-z][a-z]' $1; then
	let score=score-3
fi

if egrep -q '([A-Za-z0-9])\1+' $1; then
	let score=score-10
fi

echo "Password Score: $score"

