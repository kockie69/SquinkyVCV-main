# Grammar stuff

## Load one

Open the context menu. There is an item "Load grammar". Pick that and it should be obvious.

## Make your own

Need more detail here, but for now, this is what I got.

If you want to make a grammar, make sure you can easily see the logging output. The best way to do that is to run VCV Rack from a command line prompt. Then you will see the log output as it occurs.

There are many, many messages that may come out if you try to load a bad grammar. It's essential that you watch this, if only to see if your grammar loaded. If it loads successfully it will print out a summary of the rules.

There is an example grammar in this repo [here](../experiments/grammars). There is also a schema next to it.

## Rules a grammar must follow

The filename should be .json.

If you are not using Visual Studio Code to edit your grammar, you probably don't need to include the line

```json
"$schema": "./grammar.schema.json",
```

That is is a VS specific way of pointing to the schema. If you DO want to use VS Code, then you will probably want to include that line and make sure that grammar.schema.json is next to your file.

You probably do not need the line

```json
"line_endings": "unix",
```

Now, for some real stuff!

Here is grammar-1.json, the one this is in the repo:
```json
{
    "$schema": "./grammar.schema.json",
    "line_endings": "unix",
    "rules": [
        {
            "lhs": "q",
            "entries": [
                {
                    "rhs": [
                        "e",
                        "e"
                    ],
                    "p": 50
                },
                {
                    "rhs": [
                        "x",
                        "x",
                        "x",
                        "x"
                    ],
                    "p": 50
                }
            ]
        }
    ]
}
```

As you can see from the example, a grammar is made up of a list of production rules. Each rule has a single note on the "left hand side", and several notes on the right hand side in one or more entries. It is saying "this note on the left can be split into these notes on the right."

The sum of the metric time on the right, for any entry, must be exactly the same as the left. So if left is a half note, right could be four eight notes.

The rule that has the longest duration is the "root rule". This will determine how long each generated rhythm will be.

Every rule must be reachable, although the root rule is special, and is always reachable.

For example:

```json
half -> quarter, quarter
quarter -> eighth, eighth
```

The first rule is the root rule. It's a half note, so all the rhythms will be a half note long.

If the first rule fires, making quarter notes, then the second rule is reachable, because it has a quarter on the right hand side.

Any rule may have multiple entries (right hand sides), but always a single left hand side. For example:

```json
half -> quarter, quarter
        quarter, eighth, eighth
```

Here is an example of grammar with an unreachable rule:

```json
half -> eighth, eighth, eighth, eighth
quarter -> eighth, eighth
```

Those rules all add up correctly, but the second rule is unreachable. There is no rule that will make a quarter note, so the left hand side can ever exist.

Current note encodings are:

* h = half
* q = quarter
* e = eighth
* s (or x) = sixteenth
* dh = dotted half
* dq = dotted quarter
* de = dotted eighth
* ds (or dx) = dotted sixteenth
* 3h = triplet half
* 3q = triplet quarter
* 3e = triplet eighth
* 3s (or 3x) = triplet sixteenth

The triplets and dots are displayed a little clumsily in the UI, ATM...
