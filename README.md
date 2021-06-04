# C++ port of Moses tokenizer
This is a (currenlty briefly abandoned) work-in-progress of porting the [Moses tokenizer perl script](https://github.com/kpu/preprocess/blob/master/moses/tokenizer/tokenizer.perl) used in ParaCrawl to C++ in an attempt to roll it into [the document aligner](https://github.com/bitextor/bitextor/tree/master/document-aligner).

Shared here for future reference & back-up.

## Goals
- **Not (much) slower than the Perl equivalent**  
  Perl is really good at executing perl regular expressions! Who would have thought!
- **Just depends on boost**  
  because we already have a dependency on boost in bitextor. Could try to port it to libpcre2 instead?
- **non-breaking prefixes baked into the library**  
  no messing around with separate files in predefined locations. Just link against this library and be done with it. I'll add a feature where you can specify a prefix directory through an environment variable, where it will fall back on the build-in prefixes only when there is no real file found.

