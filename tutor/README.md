# Lab helper: ask an AI assistant about these labs

`tutor_bundle.md` in this folder is one file that holds everything about the
labs: the READMEs, the docs, `rt.h` and the source of every sketch, with
instructions at the top that tell the assistant how to help. Attach it to a
chat and you can ask about a sketch, a line of output or a README section,
and get an answer based on the actual lab code.

## How to use it

1. Download [tutor_bundle.md](tutor_bundle.md) (on GitHub: open it, then the
   download button, "Download raw file").
2. Open a **new** chat on [claude.ai](https://claude.ai). A free account is
   enough. Other assistants that accept file uploads work too.
3. Attach `tutor_bundle.md` to your first message.
4. In the same message, say which lab, sketch and key, paste the **complete**
   serial output of that run, and ask your question. For example:

   ```
   Lab 1, periodic_jitter, key e. Here is the output:

   <paste everything from "--- variant e" to the VERDICT line>

   Why is the minimum interval 9425 us when the period is 10 ms?
   ```

5. Ask follow-up questions in the same chat. Start a new chat (and attach the
   file again) for a different lab, so the answers stay focused.

## What it is for, and what it is not

It explains: what a sketch does and who calls what, what each line of your
output means, why your numbers came out the way they did, and what a README
section is saying. It answers in Romanian or English, whichever you write in.

It does not write your report. It is told not to, and the report is where
you show that you understood the experiment.

It can be wrong. If an answer contradicts the lab README or your own
output, trust the README and your board, and ask the teaching assistant.

## For maintainers

The bundle is generated. After changing a README, a doc, `common/rt.h` or a
sketch, run `make tutor` (or `tools/make_tutor_bundle.sh`) and commit the new
`tutor_bundle.md`. `tools/make_tutor_bundle.sh --check` reports whether the
committed file is out of date. The assistant's instructions are in
[rules.md](rules.md).
