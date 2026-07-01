# MazarbuLib Documentation

In-depth documentation for MazarbuLib. The project [README](../README.md) is
the quick-start entry point; the pages here go deeper into how the library is
built, how it works internally, and why it is shaped the way it is.

## Contents

| Page | Purpose |
|------|---------|
| [Architecture](architecture.md) | Data model, render pipeline, memory layout, concurrency model. |
| [API Reference](api-reference.md) | Every public function, type, and macro, with return codes and examples. |
| [Integration Guide](integration.md) | Writing `uart_send` / `screen_clear`, ISR safety, refresh rate, float printf. |
| [Tooling Guide](tooling.md) | Building, tests, sanitizers, static analysis, formatting, CI. |
| [Design Decisions](design-decisions.md) | The rationale behind the load-bearing design choices. |

## Conventions

These docs follow the same rules as the rest of the repository. Diagrams are written as
Mermaid inside fenced code blocks so they render on GitHub and stay diffable.

## Scope

MazarbuLib is a read-only tabular display over UART with polling refresh and
byte-driven navigation. It uses static allocation only and has no external
dependencies. Anything outside that scope is intentionally absent; see the
[design decisions](design-decisions.md) for the reasoning.
