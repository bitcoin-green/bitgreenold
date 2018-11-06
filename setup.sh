#!/bin/bash

cd ~
echo "****************************************************************************"
echo "* Ubuntu 16.04 is the recommended opearting system for this install.       *"
echo "*                                                                          *"
echo "* This script will install and configure your Bitcoin Green  masternodes.  *"
echo "****************************************************************************"
echo && echo && echo
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "!                                                 !"
echo "! Make sure you double check before hitting enter !"
echo "!                                                 !"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo && echo && echo

echo "Do you want to install all needed dependencies (no if you did it before)? [y/n]"
read DOSETUP

if [[ $DOSETUP =~ "y" ]] ; then
  sudo apt-get update
  sudo apt-get -y upgrade
  sudo apt-get -y dist-upgrade
  sudo apt-get install -y nano htop git
  sudo apt-get install -y software-properties-common
  sudo apt-get install -y build-essential libtool autotools-dev pkg-config libssl-dev
  sudo apt-get install -y libboost-all-dev
  sudo apt-get install -y libevent-dev
  sudo apt-get install -y libminiupnpc-dev
  sudo apt-get install -y autoconf
  sudo apt-get install -y automake unzip
  sudo add-apt-repository  -y  ppa:bitcoin/bitcoin
  sudo apt-get update
  sudo apt-get install -y libdb4.8-dev libdb4.8++-dev

  cd /var
  sudo touch swap.img
  sudo chmod 600 swap.img
  sudo dd if=/dev/zero of=/var/swap.img bs=1024k count=2000
  sudo mkswap /var/swap.img
  sudo swapon /var/swap.img
  sudo free
  sudo echo "/var/swap.img none swap sw 0 0" >> /etc/fstab
  cd

  wget https://github.com/bitcoingreen/bitcoingreen/releases/download/v1.3.0/bitcoingreen-1.3.0-x86_64-linux-gnu.tar.gz
  tar -xzf bitcoingreen-1.3.0-x86_64-linux-gnu.tar.gz
  # chmod 755 bitcoingreen-1.3.0/bin/*
  # rm /usr/bin/bitcoingreen*
  # mv bitcoingreen-1.3.0/bin/bitcoingreen* /usr/bin
  # rm -r bitcoingreen-1.3.0
  sudo mv  bitcoingreen*/bin/* /usr/bin

  sudo apt-get install -y ufw
  sudo ufw allow ssh/tcp
  sudo ufw limit ssh/tcp
  sudo ufw logging on
  echo "y" | sudo ufw enable
  sudo ufw status

  mkdir -p ~/bin
  echo 'export PATH=~/bin:$PATH' > ~/.bash_aliases
  source ~/.bashrc
fi

## Setup conf
mkdir -p ~/bin
echo ""
echo "Configure your masternodes now!"
echo "Type the IP of this server, followed by [ENTER]:"
read IP

MNCOUNT=""
re='^[0-9]+$'
while ! [[ $MNCOUNT =~ $re ]] ; do
   echo ""
   echo "How many nodes do you want to create on this server?, followed by [ENTER]:"
   read MNCOUNT
done

wget https://github.com/XeZZoR/scripts/raw/master/BITG/peers.dat -O bitg_peers.dat

for i in `seq 1 1 $MNCOUNT`; do
  echo ""
  echo "Enter alias for new node"
  read ALIAS  

  echo ""
  echo "Enter port for node $ALIAS"
  read PORT

  echo ""
  echo "Enter masternode private key for node $ALIAS"
  read PRIVKEY

  echo ""
  echo "Enter RPC Port (Any valid free port: i.E. 17100)"
  read RPCPORT

  ALIAS=${ALIAS,,}
  CONF_DIR=~/.bitcoingreen_$ALIAS

  # Create scripts
  echo '#!/bin/bash' > ~/bin/bitcoingreend_$ALIAS.sh
  echo "bitcoingreend -daemon -conf=$CONF_DIR/bitcoingreen.conf -datadir=$CONF_DIR "'$*' >> ~/bin/bitcoingreend_$ALIAS.sh
  echo '#!/bin/bash' > ~/bin/bitcoingreen-cli_$ALIAS.sh
  echo "bitcoingreen-cli -conf=$CONF_DIR/bitcoingreen.conf -datadir=$CONF_DIR "'$*' >> ~/bin/bitcoingreen-cli_$ALIAS.sh
  echo '#!/bin/bash' > ~/bin/bitcoingreen-tx_$ALIAS.sh
  echo "bitcoingreen-tx -conf=$CONF_DIR/bitcoingreen.conf -datadir=$CONF_DIR "'$*' >> ~/bin/bitcoingreen-tx_$ALIAS.sh 
  chmod 755 ~/bin/bitcoingreen*.sh

  mkdir -p $CONF_DIR
  echo "rpcuser=user"`shuf -i 100000-10000000 -n 1` >> bitcoingreen.conf_TEMP
  echo "rpcpassword=pass"`shuf -i 100000-10000000 -n 1` >> bitcoingreen.conf_TEMP
  echo "rpcallowip=127.0.0.1" >> bitcoingreen.conf_TEMP
  echo "rpcport=$RPCPORT" >> bitcoingreen.conf_TEMP
  echo "listen=1" >> bitcoingreen.conf_TEMP
  echo "server=1" >> bitcoingreen.conf_TEMP
  echo "daemon=1" >> bitcoingreen.conf_TEMP
  echo "logtimestamps=1" >> bitcoingreen.conf_TEMP
  echo "maxconnections=256" >> bitcoingreen.conf_TEMP
  echo "masternode=1" >> bitcoingreen.conf_TEMP
  echo "" >> bitcoingreen.conf_TEMP

  echo "addnode=addnode=51.15.198.252" >> bitcoingreen.conf_TEMP
  echo "addnode=addnode=51.15.206.123" >> bitcoingreen.conf_TEMP
  echo "addnode=addnode=51.15.66.234" >> bitcoingreen.conf_TEMP
  echo "addnode=addnode=51.15.86.224" >> bitcoingreen.conf_TEMP
  echo "addnode=addnode=51.15.89.27" >> bitcoingreen.conf_TEMP
  echo "addnode=addnode=51.15.57.193" >> bitcoingreen.conf_TEMP
  echo "addnode=addnode=134.255.232.212" >> bitcoingreen.conf_TEMP
  echo "addnode=addnode=185.239.238.237" >> bitcoingreen.conf_TEMP
  echo "addnode=addnode=185.239.238.240" >> bitcoingreen.conf_TEMP
  echo "addnode=addnode=134.255.232.212" >> bitcoingreen.conf_TEMP
  echo "addnode=addnode=207.148.26.77" >> bitcoingreen.conf_TEMP
  echo "addnode=addnode=207.148.19.239" >> bitcoingreen.conf_TEMP
  echo "addnode=addnode=108.61.103.123" >> bitcoingreen.conf_TEMP
  echo "addnode=addnode=185.239.238.89" >> bitcoingreen.conf_TEMP
  echo "addnode=addnode=185.239.238.92" >> bitcoingreen.conf_TEMP

  echo "" >> bitcoingreen.conf_TEMP
  echo "port=$PORT" >> bitcoingreen.conf_TEMP
  echo "masternodeaddr=$IP:$PORT" >> bitcoingreen.conf_TEMP
  echo "masternodeprivkey=$PRIVKEY" >> bitcoingreen.conf_TEMP
  sudo ufw allow $PORT/tcp

  mv bitcoingreen.conf_TEMP $CONF_DIR/bitcoingreen.conf
  cp bitg_peers.dat $CONF_DIR/peers.dat
  
  sh ~/bin/bitcoingreend_$ALIAS.sh
done