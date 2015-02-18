// Copyright & License details are available under JXCORE_LICENSE file

// This is a free list to avoid creating so many of the same object.
exports.FreeList = function(name_, max_, constructor_) {
  this.name = name_;
  this.constructor = constructor_;
  this.max = max_;
  this.list = new Array();

  this.alloc = function() {
    return this.list.length ? this.list.shift() : this.constructor.apply(this,
            arguments);
  };

  this.free = function(obj) {
    if (this.list.length < this.max) {
      this.list.push(obj);
    }
  };
};