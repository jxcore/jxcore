const test       = require('tape')
    , testCommon = require('abstract-leveldown/testCommon')
    , leveldown  = require('../')
    , abstract   = require('abstract-leveldown/abstract/ranges-test')

abstract.all(leveldown, test, testCommon)
