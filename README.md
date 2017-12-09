# ETSL: Extended Test Specification Language

Work in progress.

## Extensions to TSL

### Standard TSL

```text
Parameters:
    x:
        neg.       [property x_neg]
        zero.      [property x_zero]
        pos.       [property x_pos]

    y:
        neg.       [property y_neg]
        zero.      [property y_zero]
        pos.       [property y_pos]
        null.      [property y_null]

    result:
        NullPtrEx. [if y_null]
        ArithEx.   [if y_zero]
        zero.      [if x_zero && (!y_null && !y_zero)]
        pos.       [if x_pos && y_pos || x_neg && y_neg]
        neg.       [if x_pos && y_neg || x_neg && y_pos]
```

will produce the following result (excerpt):

```text

Test Case 1  		(Key = 1.1.4.)
   x      :  neg
   y      :  neg
   result :  pos


Test Case 2  		(Key = 1.1.5.)
   x      :  neg
   y      :  neg
   result :  neg


Test Case 3  		(Key = 1.2.2.)
   x      :  neg
   y      :  zero
   result :  ArithEx

     :
     :

Test Case 23 		(Key = 3.4.1.)
   x      :  pos
   y      :  null
   result :  NullPtrEx


Test Case 24 		(Key = 3.4.5.)
   x      :  pos
   y      :  null
   result :  neg
```

### Extended TSL

Extended TSL has the following additional features:

- Automatic property definition.
    - Property `cat_name:choice_name` is `true` when choice `choice_name` is
      selected for category `cat_name`.
    - Property `cat_name` is `true` when choice `true` is
      selected for category `cat_name`.
    - Property `:choice_name` is `true` when choice `choice_name` is
      selected for any of the categories.
- `Expected` section for describing the expected results by making choices
  mutually exclusive.
    - Only the first satisfiable choice is selected.
    - If none is satisfiable, `<n/a>` is selected.

```text
Parameters:

    x:
        neg.
        zero.
        pos.

    y:
        neg.
        zero.
        pos.
        null.

Expectatoins:

    result:
        NullPtrEx. [if y:null]
        ArithEx.   [if y:zero]
        zero.      [if x:zero]
        pos.       [if x:pos && y:pos || x:neg && y:neg]
        neg.
```

This will produce the exact same output as the standard TSL example.

## Building ETSL

Install CMake first. Then

    cmake .
    make

## Usage

Usage follows the old TSL tool for now.

    etsl [ --manpage ] [ -cs ] input_file [ -o output_file ]

## Author

- [Yutaka Tsutano](http://yutaka.tsutano.com) at University of Nebraska-Lincoln.

## License

- See [LICENSE.md](LICENSE.md) for license rights and limitations (ISC).
