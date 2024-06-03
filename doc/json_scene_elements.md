## Json Scene Elements

## Nodes

Currently we just support a string of elements by name,
or qualified name, and a color.

```json
{
    "elements" : [
        {
            "name" : "<string>",
            "color" : "#FF0044"
        }
    ]
}
```

For instance, if you have 100 elements on the view and wants to know where `bal, balber, balxml` are, with a different and more brigther color:

```json
{
    "elements" : [
        {"name" : "bal",    "color" : "#FF0000" },
        {"name" : "balber", "color" : "#FF0000" },
        {"name" : "balxml", "color" : "#FF0000" }
    ]
}
```

All of the elements on the node names, besides the name, are optional so there will not be any error if you missed something, or if you mispelled.
please make sure you are using `color` and not `colour`.
