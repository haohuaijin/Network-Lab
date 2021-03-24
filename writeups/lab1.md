Lab 1 Writeup
=============

My name: [your name here]

My SUNet ID: [your sunetid here]

This lab took me about [10] hours to do. I [did/did not] attend the lab session.

##### Program Structure and Design of the StreamReassembler:
I use `map<size_t, string>` to store the `index` and `data`.
##### Implementation Challenges:
In the overlap section. I must deal with the overlap segments. I can't find a good scheme to solve all the overlap problems. So i check the tests one by one, and solving it one by one. 

##### Remaining Bugs:
First i use the `unordered_map` to store index and data, but the test 7 always faild. Then i change it to `map`, i pass all tests.

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
