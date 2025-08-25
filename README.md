# Companion Code for My Blog

This repository contains the source code examples for my blogs [Visit Me](https://sentientbeing.bearblog.dev/blog/).

## Repository Structure

Each folder in this repository corresponds to a specific blog post. The folder name will generally match the topic of the post.

## Usage

Each project folder is self contained and includes a `Makefile` for easy compilation. To build and run any of the examples:

1.  Navigate into the project directory you're interested in:
    ```bash
    cd <folder_name>
    ```

2.  Compile the code using `make`:
    ```bash
    make
    ```

3.  Run the resulting executable:
    ```bash
    ./<executable_name>
    ```

## Environment

The code is written in C/C++ and is primarily intended for a **Linux based environment**. It often relies on POSIX specific APIs that may not be available on other operating systems without modification.