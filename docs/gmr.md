# GMR Stochastic Grammar Trigger Generator

## What does GMR do

It generates a stream of triggers by sub-dividing time randomly,  but following a grammar that the user controls. One interesting property this has is that it makes it “easy” to generate “normal” rhythms. The generated rhythms can be as random or deterministic as you like.
GMR divides time using (get ready) A set of stochastic  production rules that make up a stochastic grammar. You don’t have to know what this mean to use GMR, but if you have been exposed to formal grammars before it may be familiar.

## The essentials you need

Every production rule has a left hand side, which is the amount of time that the rule may divide. Then there is a right hand side that is the divided up time, which is a list of rhythmic durations. Then there is a probability that the rule will fire. One important thing is that the  total metric time of the left hand side and right hand side will be equal.
As a simple example:

```text
Half note -> quarter note, quarter note: 50%
```

This means that while GMR is dividing up time, if it has a half note then 50% of the time it will divide it into two quarter notes. Half the time it will just stay a half note. Every time an entire “sentence” (sequence of notes generated by the grammar) plays, a new one is generated after that.

## How to hook it up / first use

Connect an X96 clock from an impromptu module to the clock input, which is the jack on the left. The triggers will come out the jack on the right.

To get a feel for what it does, I would turn the probability of each rule down to zero. Then it will only put out a stream of half notes. The add in a little bit of one of the half note production rules, and see what happens.

## Current limitations

* The outputs are short triggers. Which is good for driving drums, but not so good for a melody or something. Having a controllable gate would be much better, imho.
* Patch data is not saved/loaded.
* It is difficult to drag the module around, or get to the context menu, because only the very bottom of the panel is “live”.
* It will only accept a X96 clock, I think it should have more clock rate options, like Seq++.
* There is only one fixed grammar in there. At the very least I would like to be able to select which rules show up in the UI.
* The one grammar is very small/limited.
* Grammars are limited to a root of a half note. Whole note or two whole notes would make more sense (to me).
* Perhaps grammars should be able to have reset, or other control of articulation.
* The grammar does not have a way to generate tuples, like a triplet. Or dotted notes.
* The UI is quite ugly.
* Should have normal clocking options: clock rate, run and reset inputs.
* There is no CV control.
* Rules are show with English spelling of notes, like "quarter". These take up too much space - would much rather use standard music notation.
* Not compatible with Stoermelder modules.

## Possible "extra" features

I want to use music notation for production rules, rather than words. Will almost certainly do this.

I would like to be able to make a production rule depend on where the left hand side occurs in the measure. For example:

```text
Quarter note on beat 3 -> four sixteenth notes.
```

* Hold (or generate) option (cv and possibly button). So that you could play the same production over and every, instead of generating a new one each time.
* Support time signatures (so whole note could 3 three quarters or 7...).

## How the UI is laid out

There is a “bottom area” where the jacks are. ATM this is the only place that acts like a panel, as far is the mouse and such are concerned.

At the top are a row of navigation tabs. They switch between different screens that fill up most of the middle area.

The “main” screen in the middle has nothing in it. Eventually it should indicate at least what beat it is on, but hopefully more.

The other screens are for production rules. With the current grammar the “initial” duration is a half note, so the first tab is all the rules for diving up a half note. Then, since the first set of rules can generate quarter notes, there is a set of rules for dividing up quarter notes.
In general the tabs will be in descending order of duration, and a tab will only appear if that metric unit is possible in the grammar.

## Grammars

This document has more details, and also describes how to make your own. [grammars](./gmr-custom.md)

## About the probabilities

Each rule has a probability next to it that is controlled by a knob. They go from 0 to 100 percent, rather than the more correct and esoteric 0 to 1. The grammar itself will have a default probability for each rule, but you will want to change these.

You might be wondering - what does it mean if the probabilities do not add up to 100%. Glad you asked!

It they add up to less than 100%, then it is quite possible that no rules will fire. In that case, then the dividing of note into smaller and smaller intervals using more and more rules will stop. Because there is always an "implicit rule" lhs -> lhs. Look back at the case, above here ha half note produces two quarter note 50%. There is an implicit (invisible) rule saying half note stops dividing 50%.

What if the probabilities add up to more than 100%? then some of the later rules may not fire. If the first rule is 50% and the second rule is 60%, then for sure one of these will fire, and no subsequent rules could possibly fire.

## More info

This module is just an implementation of an idea from Kevin Jones' thesis, published in Computer Music Journal Vol 5, Number 2, from 1981. You can read the original article on JSTOR, although you need to sign up for a free account to read it: [on JSTOR](https://www.jstor.org/stable/3679879?seq=1#metadata_info_tab_contents)

That article has some interesting information on related techniques, like generating music from Markov chains.
