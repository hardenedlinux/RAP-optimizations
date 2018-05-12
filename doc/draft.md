# algoritm

```c
// Every function pointer has a set contain the targets it's alias, the set is 
// changed ues static analysis of GCC can konwn it.

for every function pointer which has indirect call
        legal_set = pointer alias set // changed along with reachable functions
        if (legal_set->is_updated && function pointer used as indirect call)
                compute new hash depened on the future of item of current legal_set
                && insert at every item of legal_set
        // Now we have a new hash
```

*live long and prosper.*`
