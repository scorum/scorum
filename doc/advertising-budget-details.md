Advertising details {#advdetails}
===================

Here are some details and insights abount how advertising budgets work.

## Table of contents
* [How start_time and deadline will be aligned to blockchain interval?](#advalignment)
* [How per-block payment is evaluated?](#advperblock)
* [What if you do not specify start_time?](#advemptystart)
* [What is a budget's cashout and how it works?](#advcashout)
* [What if budget's start_time equal deadline (start and end are within same block)?](#advstartendsameblock)
* [How advertising auction works?](#advauction)
* [Example](#advexample)

<a name="advalignment"></a>

## How start_time and deadline will be aligned to blockchain interval?

User can specify arbitrary `start_time` and `deadline` with no reference to blockchain blocks interval. So we need to align (if they weren't aligned) these parameters by blockchain block interval which is equal 3 sec for now.

Let's say genesis time (block #0) is 0 sec and each following block time is greater then previous block time for 3 sec.

|                   |       |       |       |       |       |       |
|------------------:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|
| Block time (+sec) |   +0  |   +3  |   +6  |   +9  |  +12  |  +15  |
|           Block # |   0   |   1   |   2   |   3   |   4   |   5   |

Let's say `start_time` equals 2sec and `deadline` equals 10sec, i.e. `start_time` is in between block #0 and block #1 and `deadline` is in between block #3 and block #4. In such cases both `start_time` and `deadline` will be aligned by greater bound i.e. in this case budget will start in block #1 and ends in block #4.

|                   |       |       |       |       |        |       |
|------------------:|:-----:|:-----:|:-----:|:-----:|:------:|:-----:|
| Block time (+sec) |   +0  |   +3  |   +6  |   +9  |   +12  |  +15  |
|           Block # |   0   |   1   |   2   |   3   |    4   |   5   |
|     Budget bounds |       | start |   -   |   -   | finish |   5   |
|      When created |   x   |       |       |       |        |       |

<a name="advperblock"></a>

## How per-block payment is evaluated?

Let's say we have already aligned `start_time` and `deadline`. Is they are not then we will align them using method described [start_time and deadline alignment](#advalignment) section.

In order to evaluate per_block payment we need to divide budget balance on per_block payments count which, in turn, equals:

```cpp
((deadline - start_time) / block_interval) + 1
```
I.e. per block payment equals:
```cpp
per_block = balance / (((deadline - start_time) / block_interval) + 1)
```

Fox example, if budget balance equals 100 SCR, `start_time` equals 3sec and `deadline` equals 12 sec, then there will be 4 payments by 25 SCR each in blocks #1,#2,#3,#4.

> NOTE:
> Payments are also occurred in block #1 where budget starts and in block #4 where budgets ends.

|                   |       |       |       |       |        |       |
|------------------:|:-----:|:-----:|:-----:|:-----:|:------:|:-----:|
| Block time (+sec) |   +0  |   +3  |   +6  |   +9  |   +12  |  +15  |
|           Block # |   0   |   1   |   2   |   3   |    4   |   5   |
|     Budget bounds |       | start |   -   |   -   | finish |       |
|    Budget balance |  100  |   75  |   50  |   25  |    0   |       |
|      When created |   x   |       |       |       |        |       |
|      When started |       |   x   |       |       |        |       |
|  Payment occurred |       |   x   |   x   |   x   |    x   |       |
|    Payment amount |       |   25  |   25  |   25  |   25   |       |

<a name="advemptystart"></a>

## What if you do not specify start_time?

In this case `start_time` will equal to last block head time. If no blocks were missed then `start_time` will equal previous block head time relative to current applying block timestamp.

Let's say we are creating budget in block #2 (+6 sec) with no specified `start_time` and `deadline` which equals 12 sec. In this case `start_time` corresponds to previous block i.e. to block #1 and equals 3 sec.

|                   |       |       |       |       |        |       |
|------------------:|:-----:|:-----:|:-----:|:-----:|:------:|:-----:|
| Block time (+sec) |   +0  |   +3  |   +6  |   +9  |   +12  |  +15  |
|           Block # |   0   |   1   |   2   |   3   |    4   |   5   |
|     Budget bounds |       | start |   -   |   -   | finish |       |
|      When created |       |       |   x   |       |        |       |
|      When started |       |       |   x   |       |        |       |
|   Payment occured |       |       |   x   |   x   |    x   |       |
|    Budget balance |       |       |   75  |   50  |   25   |       |
|    Payment amount |       |       |   25  |   25  |   25   |       |

> NOTE:
> * Per block payment is evaluated to 25 SCR (based on `start_time` and `deadline`)
> * Budget balance isn't empty after 3rd payment and 25 SCR should be returned to budget's owner.

If previous block is missed then `start_time` will be equal 0 sec. In this case per block payment will be equal `[100 / ((12 - 0) / 3 + 1)]` i.e. 20 SCR. 40 SCR will be returned to owner after budget will close.

|                   |       |          |       |       |        |       |
|------------------:|:-----:|:--------:|:-----:|:-----:|:------:|:-----:|
| Block time (+sec) |   +0  |    +3    |   +6  |   +9  |   +12  |  +15  |
|           Block # |   0   | *missed* |   1   |   2   |    3   |   4   |
|     Budget bounds | start |     -    |   -   |   -   | finish |       |
|      When created |       |          |   x   |       |        |       |
|      When started |       |          |   x   |       |        |       |
|   Payment occured |       |          |   x   |   x   |    x   |       |
|    Budget balance |       |          |   80  |   60  |   40   |       |
|    Payment amount |       |          |   20  |   20  |   20   |       |

> We can say that `start_time` means that budget won't start before this time but it could start after this timestamp because of missed blocks.

<a name="advcashout"></a>

## What is a budget's cashout and how it works?

In order to optimize budget payments events count and do not overload blockchain with tons of events we decided to accumulate all payments from advertising auction in special pending buffers. There are two of them in each budget. The first one accumulates SCR which should be returned to owners account(owner_pending_income). The second one accumulates SCR which should be transferred to dev pool and activity reward pool (budget_pending_outgo). Each week starting from previous cashout all cash from first pending buffer goes to budget's owner and all cash from second pending buffer is distributed across dev pool and activity reward pool.

If `deadline` occurs earlier than next cashout then we are performing cashout immediately in current block and closing budget.

<a name="advexample"></a>

Let's say we are starting budget with such specs:
* start in block #1
* `deadline` in block #12
* cashout time equals 15 sec
* owners balance equals 2000 SCR
* budgets balance equals 1210 SCR
* per block payment is equal `[1210 / ((36 - 3) / 3 + 1)]` i.e. 100 SCR
* let's say that according to auction algorithm 80% goes to dev pool and activity reward pool and 20% returns to budget's owner

> NOTE:
> Budget balance do not divide completely on per block counts so the rest (which is equal 10 SCR) will be returned to owner after budget will be closed.


|                   |       |       |       |       |       |       |       |       |       |       |       |       |        |       |
|------------------:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:-----:|:------:|:-----:|
| Block time (+sec) |   +0  |   +3  |   +6  |   +9  |  +12  |  +15  |  +18  |  +21  |  +24  |  +27  |  +30  |  +33  |   +36  |  +39  |
|           Block # |   0   |   1   |   2   |   3   |   4   |   5   |   6   |   7   |   8   |   9   |   10  |   11  |   12   |   13  |
|     Budget bounds |       | start |   -   |   -   |   -   |   -   |   -   |   -   |   -   |   -   |   -   |   -   | finish |       |
|      When created |   x   |       |       |       |       |       |       |       |       |       |       |       |        |       |
|   Payment occured |       |   x   |   x   |   x   |   x   |   x   |   x   |   x   |   x   |   x   |   x   |   x   |    x   |       |
|   Cashout occured |       |       |       |       |       |   x   |       |       |       |       |   x   |       |    x   |       |
|      Budget outgo |       |   80  |  160  |  240  |  320  |   0   |   80  |  160  |  240  |  320  |   0   |   80  |    0   |       |
|      Owner income |       |   20  |   40  |   60  |   80  |   0   |   20  |   40  |   60  |   80  |   0   |   20  |    0   |       |
|    Budget balance |  1210 |  1110 |  1010 |  910  |  810  |  710  |  610  |  510  |  410  |  310  |  210  |  110  |    0   |       |
|     Owner balance |  790  |  790  |  790  |  790  |  790  |  890  |  890  |  890  |  890  |  890  |  990  |  990  |  1040  |       |
| Dev + reward pool |   0   |   0   |   0   |   0   |   0   |  400  |  400  |  400  |  400  |  400  |  800  |  800  |   960  |       |

<a name="advstartendsameblock"></a>

## What if budget's start_time equals deadline (start and end are within same block)?

First of all this is a valid case and budget will be created.

Per block payment will equal all budget's balance.
Such budget can participate advertising auction.

After auction such budget will be closed.

<a name="advauction"></a>

## How advertising auction works?

Our web platform has various places to put advertising on it. Some of these places are better, some are worse. Advertisers which are ready to pay max value obtain better place for its' ads. Each advertising place has its own coefficient from 0 to 100.

Algorithm:
1. Getting top budgets ordered by its' per block payment amount in ascending order
2. Getting ads coefficients ordered in ascending order
3. Evaluating bet for each budget using recurrent formula:
    - bet[n] = per_block[n-1], n = 0, per_block[n-1] exists
    - bet[n] = per_block[n], n = 0, per_block[n-1] do not exist
    - bet[n] = min(bet[n-1] + (coeff[n] - coeff[n-1])/coeff[MAX-1], per_block[n]), n = 1..MAX-1
    - MAX    = min(ads places count, budgets count)