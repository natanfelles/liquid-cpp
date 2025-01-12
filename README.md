
## Quickstart

Install with (on Debian/Ubuntu-like systems):

```bash
sudo apt-get -y install git build-essential g++ cmake
git clone https://github.com/adamharrison/liquid-cpp.git && cd liquid-cpp && mkdir -p build && cd build && cmake .. && make -j 4 && sudo make install
```

Build with: `g++ program.cpp -lliquid`

```c++
#include <iostream>
#include <liquid/liquid.h>

int main(int argc, char* argv[]) {
    Liquid::CPPVariable store;
    store["a"] = 10;
    std::string tmpl = "{% if a > 1 %}123423{% else %}sdfjkshdfjkhsdf{% endif %}";

    Liquid::Context context(Liquid::Context::EDialects::PERMISSIVE_STANDARD_DIALECT);
    std::cout << Liquid::Renderer(context).render(Liquid::Parser(context).parse(tmpl), store) << std::endl;
    return 0;
}
```

## Introduction

A fully featured C++17 parser, renderer and optimizer for [Liquid](https://shopify.github.io/liquid/); Shopify's templating language. It's designed to provide official support for using liquid in the following languages:

* C++
* C
* Ruby
* Perl

Other languages (Javascript, Python, etc..) may come later, but any support for them will be unofficial.

## Goals

Here's the overriding philosophy of what I'm aiming for, for this library.

### Modular

All components of the process should be decoupled, and as much control as possible should be transferred to the programmer; nothing should be a monolith.

### Extensible

You should be able to modify almost everything about the way the language behaves by registering new tags, operators, filters, drops, dialects; very little
should be in the core.

### General

There should be no special cases; every instance should be part of a generalizable set of classes, and operators. No direct text parsing, unless you absolutely need to.

### Performant

It should be at least an order of magnitude faster than Ruby liquid, preferably more. It should also use at most an order of magnitude less memory.

### Portable

It should be dead-easy to implement liquid for a new programming language, which means a really robust and easy-to-use C interface that can access most, if not all parts of the system.

It should also compile on Windows, Mac, and Linux easily.

### Independent

It should have basically no dependencies, other than the C++ standard library itself. There is a small dependency of `libcrypto` if you compile the `Web` dialect, but it is not included
by default.

### Conformance

It should pass all Shopify's test suites, and act exactly as Shopify liquid, if configured to do so.

### Insanity at the Edges

That being said, the deficiencies of the Shopify implementation shouldn't hold it back. It should be extremely easy to toggle extra features;
and a single call should permit `permissive` liquid, which allows for things like expression evaluation in all contexts. All things associated
with the ruby version of Liquid, like global state, and whatnot, should exist purely as a façade pattern in a standalone Ruby module, which
requires an underlying ruby module that implements the more modular, sane version.

## Status

In development. Mostly stable. **VM/Compiler/Interpreter, should not be used in production code, yet, though the rest of the library probably can be, though I make no guarantees.**

Basic memory audits of the core have been done with valgrind, and leaks have been closed. A more extensive battery of tests is required to determine if there's actually any undefined behaviour or further leaks.

### Quick Start

### Building

Building the library is easy; so long as you have the appropriate build-system; currently the library uses cmake to build across platforms.

It can be built and installed like so, from the main directory:

```mkdir -p build && cd build && cmake .. && make && sudo make install```

Eventually, I'll have a .deb that can be downloaded from somewhere for Ubuntu distros, but that's not quite up yet.

#### C++

The C++ library, which is built with the normal Makefile can be linked in as a static library. Will eventually be available as a header-only library.

```c++
#include <iostream>
#include <liquid/liquid.h>

int main(int argc, char* argv[]) {
    // The liquid context represents all registered tags, operators, filters, and a way to resolve variables.
    Liquid::Context context;
    // Very few of the bits of liquid are part of the 'core'; instead, they are implemented as dialects. In order to
    // stick in most of the default bits of Liquid, you can ask the context to implement the standard dialect, but this
    // is not necessary. The only default tag available is {% raw %}, as this is less a tag, and more a lexing hint. No
    // filters, or operators, other than the unary - operator, are available by default; they are are all part of the liquid standard dialect.
    // In addition to setting all these nodes up, this also sets the 'truthiness' of variables to evaluted in a loose manner; meaning
    // that in addition to false and nil, being not true; 0, and empty string are also considered not true.
    // In order to implement the stricter Shopify ruby version of things, use `implementStrict` instead.
    Liquid::StandardDialect::implementPermissive(context);
    // In addition, dialects can be layered. Implementing one dialect does not forgo implementating another; and dialects
    // can override one another; whichever dialect was applied last will apply its proper tags, operators, and filters.
    // Currently, there is no way to deregsiter a tag, operator, or filter once registered.

    // Initialize a parser. These should be thread-local. One parser can parse many files.
    Liquid::Parser parser(context);

    const char exampleFile[] = "{% if a > 1 %}123423{% else %}sdfjkshdfjkhsdf{% endif %}";
    // Throws an exception if there's a fatal parsing error. Liquid accepts quite a lot by default, so normally this'll be fine.
    // You can query a vector of errors under `parser.errors`.
    Liquid::Node ast = parser.parse(exampleFile, sizeof(exampleFile)-1);
    // Initialize a renderer. These should be thread-local. One renderer can render many templates.
    // Register the standard, out of the box variable implementation that lets us pass a type union'd variant that can hold either a long long, double, pointer, string, vector, or unordered_map<string, ...> .
    // All renders should be thread local.
    Liquid::Renderer renderer(context, Liquid::CPPVariableResolver());

    Liquid::CPPVariable store;
    store["a"] = 10;

    std::string result = renderer.render(ast, store);
    // Should output `123423`
    std::cout << result << std::endl;
    return 0;
}
```

This program should be linked with `-lliquid`.

#### C

The following program below is analogous to the one above, only using the external C interface.

```c

#include <stdio.h>
#include <liquid/liquid.h>

int main(int argc, char* argv[]) {
    LiquidContext context = liquidCreateContext();
    liquidImplementPermissiveStandardDialect(context);

    LiquidParser parser = liquidCreateParser(context);
    const char exampleFile[] = "{% if a > 1 %}123423{% else %}sdfjkshdfjkhsdf{% endif %}";
    LiquidLexerError lexerError;
    LiquidParserError parserError;
    LiquidTemplate tmpl = liquidParserParseTemplate(parser, exampleFile, sizeof(exampleFile)-1, NULL, &lexerError, &parserError);
    if (lexerError.type) {
        char buffer[512];
        liquidGetLexerErrorMessage(lexerError, buffer, sizeof(buffer));
        fprintf(stderr, "Lexer Error: %s\n", buffer);
        exit(-1);
    }
    if (parserError.type) {
        char buffer[512];
        liquidGetParserErrorMessage(parserError, buffer, sizeof(buffer));
        fprintf(stderr, "Parser Error: %s\n", buffer);
        exit(-1);
    }
    // This object should be thread-local.
    LiquidRenderer renderer = liquidCreateRenderer(context);


    // If no LiquidVariableResolver is specified; an internal default is used that won't read anything you pass in, but will funciton for {% assign %}, {% capture %} and other tags.
    /* LiquidVariableResolver resolver = {
        ...
    };

    liquidRegisterVariableResolver(renderer, resolver); */

    // Use something that works with your language here; as resolved by the LiquidVariableResolver above.
    void* variableStore = NULL;
    LiquidRendererError rendererError;
    LiquidTemplateRender result = liquidRendererRenderTemplate(renderer, variableStore, tmpl, &rendererError);
    if (rendererError.type) {
        char buffer[512];
        liquidGetRendererErrorMessage(rendererError, buffer, sizeof(buffer));
        fprintf(stderr, "Renderer Error: %s\n", buffer);
        exit(-1);
    }

    // Should output `sdfjkshdfjkhsdf`.
    fprintf(stdout, "%s\n", liquidTemplateRenderGetBuffer(result));

    // All resources, unless otherwise specified, must be free explicitly.
    liquidFreeTemplateRender(result);
    liquidFreeRenderer(renderer);
    liquidFreeTemplate(tmpl);
    liquidFreeContext(context);

    return 0;
}
```

This program should be linked with `-lliquid -lstdc++ -lm`.

#### Ruby

The ruby module uses the C interface to interface with the liquid library.

##### Install

Currently the package isn't uploaded on rubygems, so it has to be build manually. Luckily; this is easy:

```cd ruby/liquidcpp && gem install rake-compiler && rake compile && rake gem && gem install pkg/*.gem && cd - && cd ruby/liquidc-dir && rake compile && rake gem && gem install pkg/*.gem && cd -```

##### Usage

There're two ways to get the ruby library working. You can use the OO way, which mirrors the C++ API.

```ruby
require 'liquidcpp'
context = LiquidCPP.new()
parser = LiquidCPP::Parser.new(context)
renderer = LiquidCPP::Renderer.new(context)
template = parser.parseTemplate("{% if a %}asdfghj {{ a }}{% endif %}")
puts renderer.render({ "a" => 1 }, template)
```

Or, alternatively, one can use the "drop in replacement" module, which wraps all this, which will register the exact same constructs as the normal `liquid` gem.
The advantage of this is that by simply replacing your require statements, you should be able to use your existing code; but it'll be close to an order of magnitude
faster. The gem is currently under construction, and may not match exact Shopify gem behaviour in all circumstances.

```ruby
require 'liquidcpp-dir'

template = Liquid::Template.parse("{% if a %}asdfghj {{ a }}{% endif %}")
puts template.render({ "a" => 1 }, template)
```

This is generally discouraged, as you lose object-orientation and modularity; like the ability to have independent liquid contexts with different
tags, filters, operators, and settings vs. some weird mishmash where just shove everything into a global namespace, and "act" like you're using some OO.
But, it's of course, up to you.

#### Perl

The perl module uses the C interface to interface with the liquid library, and attempts to mimic `WWW::Shopify::Liquid`.

##### Install

Currently the module hasn't been uploaded to CPAN, but can be built and installed like so;

```cd perl && perl Makefile.PL && make && sudo make install && cd -```

Will eventually upload this.

##### Usage

Uses the exact same interface as `WWW::Shopify::Liquid`, and is basically almost fully compatible with it as `WWW::Shopify::Liquid::XS`.

All core constrcuts are overriden, and there is no optimizer at present, but all top-level functions should work correctly; and most dialects that have tags, filters and operators,
that use `operate` instead of `process` or `render` should function without changes.

```perl
    use WWW::Shopify::Liquid::XS;

    my $liquid = WWW::Shopify::Liquid::XS->new;
    my $text = $liquid->render_file({ }, "myfile.liquid");
    print "$text\n";
```

#### Python

This will probably come around at some point; but currently, there are no bindings for Python.

#### Javascript

This *may* happen. *Maybe*.

## Features / Roadmap

This is what I'm aiming for at any rate.

### Done

* Includes a standard dialect that contains all array, string and math filters by default, as well as all normal operators, and all control flow, iteration and variable tags.
* Significantly less memory usage than ruby-based liquid.
* Contextualized set of filters, operators, and tags per liquid object instantaited; no global state as in the regular Shopify gem, easily allowing for many flavours of liquid in the same process.
* Ability to easily specify additions of filters, operators, and tags called `Dialects`, which can be mixed and matched.
* Small footprint. Aiming for under 5K SLOC, with full standard Liquid as part of the core library.
* Fully featured `extern "C"` interface for easy linking to most scripting languages. OOB bindings for both Ruby and Perl will be provided, that will act as drop-in replacements for `Liquid` and `WWW::Shopify::Liquid`.
* Significant speedup over ruby-based `liquid` and `liquid/c`. (Need to do more comprehensive benchmarks; but at first glance seems like somewhere between a 10-50x speedup over regular `liquid/c` for both rendering and parsing using the VM.)
* Fully compatible with both `Liquid`, Shopify's ruby gem, and `WWW::Shopify::Liquid`, the perl implementation.
* Use a standard build system; like cmake.
* Optional compatibilty with rapidjson to allow for easy JSON reading in C++.
* Line accurate, and helpful error messages.
* Ability to step through and examine the liquid AST.
* In ruby integration, allow for all types of strict/lax modes.
* Allow for tag registration in the drop in replacement module for Ruby.
* Built-in optimizer that will do things like loop unrolling, conditional elimiation, etc..
* Togglable extra features, such as real operators, parentheses, etc..
* Ability to set limits on memory consumed, and time spent rendering.
* Ability to partially render content, then spit back out the remaining liquid that genreated it, based on an AST tree.
* UTF-8 aware.

### Partial

* Full test suite that runs all major examples from Shopify's doucmentation. (Test suite runs some examples, but not all).
* Write a register-based bytecode compiler/interpreter, which should be significantly faster than walking the parse tree. (basics in, if statements, for statements, and genrealized non-assembly calling in).

### TODO

* General polish pass to clean up reundant code, and ensure consistency across the C, C++, Perl and Ruby APIs.

## License

### MIT License

Copyright 2021 Adam Harrison

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

