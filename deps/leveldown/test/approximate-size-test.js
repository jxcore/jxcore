const test       = require('tape')
    , testCommon = require('abstract-leveldown/testCommon')
    , leveldown  = require('../')
    , abstract   = require('abstract-leveldown/abstract/approximate-size-test')

abstract.all(leveldown, test, testCommon)
