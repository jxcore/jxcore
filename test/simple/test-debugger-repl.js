// Copyright & License details are available under JXCORE_LICENSE file

console.error('Skipping: not supported on any platform.');
process.exit(0);

var repl = require('./helper-debugger-repl.js');

repl.startDebugger('breakpoints.js');

var addTest = repl.addTest;


// Next
addTest('n', [
  /break in .*:11/,
  /9/, /10/, /11/, /12/, /13/
]);

// Watch
addTest('watch("\'x\'")');

// Continue
addTest('c', [
  /break in .*:5/,
  /Watchers/,
  /0:\s+'x' = "x"/,
  /()/,
  /3/, /4/, /5/, /6/, /7/
]);

// Show watchers
addTest('watchers', [
  /0:\s+'x' = "x"/
]);

// Unwatch
addTest('unwatch("\'x\'")');

// Step out
addTest('o', [
  /break in .*:12/,
  /10/, /11/, /12/, /13/, /14/
]);

// Continue
addTest('c', [
  /break in .*:5/,
  /3/, /4/, /5/, /6/, /7/
]);

// Set breakpoint by function name
addTest('sb("setInterval()", "!(setInterval.flag++)")', [
  /1/, /2/, /3/, /4/, /5/, /6/, /7/, /8/, /9/, /10/
]);

/*
// Continue
addTest('c', [
  /break in node.js:\d+/,
  /\d/, /\d/, /\d/, /\d/, /\d/
]);
*/

addTest('quit', []);