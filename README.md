# Robot remotely controlled with fastDDS

## Robot
Zetabot https://github.com/berndporr/zetabot with Debian Trixie on it.
Install the drivers from its "wheeleddrive" subdir.

For fastDDS install these packages on the robot:
```
apt install libfastcdr-dev libfastdds-dev fastddsgen fastdds-tools
```

Then compile the robot fastDDS subscriber with:
```
cmake .
make
```

## Steering wheel
The steering wheel runs under Ubuntu and is in [steering_wheel](steering_wheel). Follow the instructions in this subdir how to compile it.

## How to run

On the Ubuntu system run `./steering wheel`.

On the robot run: `./fastDDSrobot`.

## Credit

Bernd Porr
