#!/usr/bin/env bash


# It seems to be a litte tricky to get the Sphinx automatic documentation
# to host as a web page on git pages.
#
# What we will do intead is manually synchronize the documentation
# builds with the branches of these projects used for the web pages
#
# Run this script from it's local directory
#

shopt -s extglob
git pull
# hopefully removes everything that shouldn't be being tracked as per
# .gitignore. Possibly dangerous. 
cat .gitignore | awk "/^[.\*]/" | sed 's/"/"\\""/g;s/.*/"&"/' |  xargs -E '' -I{} git rm -rf --cached {}
git rm -rf --cached *.pyc
git add . 
git add -u :/
git commit -m "${1:-"..."}"
git push origin master
