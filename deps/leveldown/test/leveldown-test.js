const test      = require('tape')
    , leveldown = require('../')
    , abstract  = require('abstract-leveldown/abstract/leveldown-test')

abstract.args(leveldown, test)
