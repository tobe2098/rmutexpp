# rmutexpp
A header only library for Rust-like mutex guard behavior



## Features to include 
- Access data via structured binding and tuple::tie
- Iterate over tuple in range-based for loop, No, different types
- Check that you own before releasing data
- Check that you own in destructor (for trylocks)
- Allow to trylock or lock again (only if not owned)
- Write all idioms in main.cpp so they can be read and seen to work

- Allow .lock() and .try_lock() and .unlock(). Get var reference from it that unlocks on destruction as well.
- If mutex is const... why use mutex? It is const. DO not construct if const, give an error saying you should just access with const label.