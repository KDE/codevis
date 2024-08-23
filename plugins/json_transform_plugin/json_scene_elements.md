## Json Scene Elements

This JSON based API is modeled after filter / map that exists in many languages.
The idea is that you pass a series of filter / map values with your query, that will
change, non-destructively, the elements on the view to help visualize the contents.

## API:

```json

 [
    {
        "filter" : {
            "text" : "regexp",
            "qualified_name" : "regexp",
            "color" : "hex-value",
            "is_empty": "boolean",
            "is_expanded": "boolean",
            "has_unloaded_elements" : "boolean",
    },
        "map" : {
            "text" : "regexp|string",
            "color" : "#FF0044",
        }
    }
]
```

For instance, if you have 100 elements on the view and wants to know where `bal, balber, balxml` are, with a different and more brigther color, and remove `BloombergLP::` from all nodes:
See the use of a capture group on the map / name. The capture group will be used to replace the text of the node to what's captured.

```json
[
    {
        "filter" : {"text" : "bal|balber|balxml"},
        "map" : {"color" : "#FF0000" },
    },
    {
        "filter" : {"text" : "BloombergLP*"},
        "map" : {"text": "BloombergLP::(.*)" },
    }
]
```

All of the elements on the node names, besides the name, are optional so there will not be any error if you missed something, or if you mispelled.
please make sure you are using `color` and not `colour`.
