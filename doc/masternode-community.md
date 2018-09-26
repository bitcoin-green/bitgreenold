Masternode Community Vote API
=============================

Bitcoin Green now supports full decentralized community vote system.

Community Votes go through a series of stages:
* prepare - create a special transaction that destroys coins in order to make a voting proposal (currently 25 BITG)
* submit - propagate transaction to peers on network
* voting - lobby for votes on your proposal


Prepare collateral transaction
------------------------------

preparecommunityproposal \<proposal-name\> \<proposal-description\> \<block_end\>

Example:
```
preparecommunityproposal "prop-name" "prop-description" 10000
```

Output: `ba0f54edc735666c7e6263f8b189d52bd1e001e523b465cd966f841f7b292b4e` - This is the collateral hash, copy this output for the next step

In this transaction we prepare collateral for "_prop-name_".

**Warning -- if you change any fields within this command, the collateral transaction will become invalid.**

Submit proposal to network
--------------------------

submitcommunityproposal  \<proposal-name\> \<proposal-description\> \<block_end\> \<collateral_hash\>

Example:
```
submitcommunityproposal "prop-name" "prop-description" 10000 ba0f54edc735666c7e6263f8b189d52bd1e001e523b465cd966f841f7b292b4e
```

Output: `c42a8e70cab014dbf6de6abc6558b465a9c3a0d7bd3806d40b9331c802d37453` - This is your proposal hash, which other nodes will use to vote on it

Lobby for votes
---------------

Double check your information:

getcommunityinfo \<proposal-name\>

Example:
```
getcommunityinfo prop-name
```
Output:
```
[
  {
    "Name": "prop-name",
    "Description": "prop-description",
    "Hash": "c42a8e70cab014dbf6de6abc6558b465a9c3a0d7bd3806d40b9331c802d37453",
    "FeeHash": "ba0f54edc735666c7e6263f8b189d52bd1e001e523b465cd966f841f7b292b4e",
    "BlockEnd": 10000,
    "Ratio": 0,
    "Yeas": 0,
    "Nays": 0,
    "Abstains": 0,
    "IsEstablished": true,
    "IsValid": true,
    "IsValidReason": "",
    "fValid": true
  }
]
```

If everything looks correct, you can ask for votes from other masternodes. To vote on a proposal, load a wallet with _masternode.conf_ file. You do not need to access your cold wallet to vote for proposals.

mncommunityvote \<mode\> \<proposal_hash\> [yes|no] (\<alias\>)

Example:
```
mncommunityvote \<mode\> c42a8e70cab014dbf6de6abc6558b465a9c3a0d7bd3806d40b9331c802d37453 yes
```

mode:
    'local' for voting directly from a masternode
    'many' for voting with a MN controller and casting the same vote for each MN
    'alias' for voting with a MN controller and casting a vote for a single MN

alias is an optional parameter with the MN alias you have in your _masternode.conf_ file.

Output: `Voted successfully` - Your vote has been submitted and accepted.

Community Vote expiration
-------------------------

When block defined in BlockEnd (in this example `10000`) is reached, your community proposal will expire.


RPC Commands
------------------------

The following new RPC commands are supported:

- preparecommunityproposal
- submitcommunityproposal
- getcommunityinfo
- checkcommunityproposals
- getcommunityproposalvotes
- mncommunityvote
