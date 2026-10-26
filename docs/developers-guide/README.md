# MINIX3x Developers Guide

This guide collects practical information for developing and contributing to
this MINIX 3 repository. It complements the existing manual pages and
repository-specific instructions in the main [README](../../README.md).

## Contents

- [Overview of Minix3x architecture](#overview-of-minix3x-architecture)

## Overview of Minix3x architecture

Minix 3 is a POSIX-compatible operating system. It is a micro-kernel running a collection of multiple user-mode server processes providing services to user-land programs and users.

The goal of its architecture is to achieve high reliability by running (almost) everything as user-mode processes, observing the principle of least authority by limiting what each process has access to and what they can do.