#ifndef __MATCH_H__
#define __MATCH_H__

// "[g/Aa\_]_t [g/Aa/1,16]([l/int,char] [g/Aa\_]);"

// "[^l/bd_[g/Aa]_t] [g/Aa\_] = ([r/0])([g/*]);"
// bd_foo_t bar = (bd_foo_t)(tee);

// [l/...] -> List match
//   Just a list of options separated by commas, empty options are allowed. Options may also consist of other kinds of matches, but
//   format specifiers will *not* work in these.
// [g/...] -> Global match
//   A sequence of set specifiers (A -> Uppercase letters, a -> Lowercase letters, _ -> Spaces and tabs, + -> Symbols, 0 -> Digits) or
//   concrete and individual characters (\ followed by the character, eg.: [g/\x\y\z] will match xzxyzz but not xabxx). Global matches
//   might also contain a repeat count specifier preceded by a /, with two numbers specifying the minimum and maximum (there will not
//   be a maximum if only a single number is specified, and default behaviour is to match any amount or just none), eg.: [g/Aa/3,7]
//   will match any amount of uppercase and lowercase letters from 3 to 7, both inclusive.
// [r/...] and ^  -> Replace match and format specifier
//   With format specifiers preceding the starting symbol of a list or global match (eg.: [^g/Aa] or [^l/foo,bar,tee]), you can later
//   use the same matches in other posterior parts of the match query or even in other strings as a replace function. To format a match
//   to use a previous match, you can do something as follows: [r/INDEX], where INDEX is a 0-starting index corresponding to the format
//   specifier of the match in use.
// As a worthy note, you can use the symbols [] in any regular query by preceding them with \, and you can use a \ in those queries by
// using \\. Additionally, you can use * in global matches to include all character dictionaries (Aa_+0). Finally, global matches will
// try to match as many characters as possible (inside the global match, *not* in total, but if a shorter global match makes the whole
// match possible then it will be preferred). List matches, in the other hand, will pick the first match that allows for a total match.

int mt_match(const char *text, int length, const char *query, const char *replace_input, char *replace_output); // Returns length of match, or 0 if it could not match

#endif
