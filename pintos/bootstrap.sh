sudo apt-get update
sudo apt-get install -y gcc g++ gdb binutils \
     libxrandr2 libxrandr-dev \
     libncurses5-dev libncurses5 
cat <<EOF > /home/vagrant/.bash_profile
PATH=/pintos-env/bin/:\$PATH
PATH=/pintos-env/pintos/utils:\$PATH
export PATH
export PS1='\${debian_chroot:+(\$debian_chroot)}\[\e[1;31m\]\u@\h:\[\e[0m\]\w\\$ '

alias ls='ls -la'
alias vagrant='echo "Closing the connection for you..."; logout'
export LC_CTYPE=en_US.UTF-8
export LC_ALL=en_US.UTF-8
EOF
