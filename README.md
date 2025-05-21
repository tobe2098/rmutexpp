# rmutexpp
A header only library for Rust-like mutex guard behavior



## Features to include 
- Access data via structured binding and tuple::tie
- Iterate over tuple in range-based for loop, No, different types
- Check that you own before releasing data
- Check that you own in destructor (for trylocks)
- Allow to trylock or lock again (only if not owned)
- Apply for each of a function (function has to be templated?)
- Apply function to all objects (if locked)
- Allow .lock() and .try_lock() and .unlock(). Get var reference from it that unlocks on destruction as well.
- Write all idioms in main.cpp so they can be read and seen to work