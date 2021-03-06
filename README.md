# ft_ping

### Prepare environment

```bash
docker build -t remote -f Dockerfile.remote .
docker run -d --cap-add sys_ptrace -p127.0.0.1:2222:22 --name remote_env remote
ssh-keygen -f "$HOME/.ssh/known_hosts" -R "[localhost]:2222"
```


### Usage

```zsh
./ft_ping google.com
```

```zsh
PING google.com (173.194.220.102) 56(84) bytes of data.
64 bytes from lk-in-f102.1e100.net (173.194.220.102): icmp_seq=1 ttl=37 time=13.59 ms
64 bytes from lk-in-f102.1e100.net (173.194.220.102): icmp_seq=2 ttl=37 time=13.84 ms
```


Default message ping `echo`s is 84 bytes length.
We can track how does our ping do in other terminal:

```zsh
# 84 coud be changed dependind ths -s [size] option (e.g. for -s 10 you need "ip[2:2] == 38")
tcpdump -x "ip[2:2] == 84"  
```

```zsh
12:17:08.174930 IP 2538cd664b5e > lf-in-f113.1e100.net: ICMP echo request, id 3070, seq 1, length 64
	0x0000:  4500 0054 fe0b 0000 4001 eb2f ac11 0002
	0x0010:  40e9 a471 0800 7ecf 0bfe 0001 44f8 e260
	0x0020:  0000 0000 16ab 0200 0000 0000 4242 4242
	0x0030:  4242 4242 4242 4242 4242 4242 4242 4242
	0x0040:  4242 4242 4242 4242 4242 4242 4242 4242
	0x0050:  4242 4242
12:17:08.188282 IP lf-in-f113.1e100.net > 2538cd664b5e: ICMP echo reply, id 3070, seq 1, length 64
	0x0000:  4500 0054 8a73 0000 2501 79c8 40e9 a471
	0x0010:  ac11 0002 0000 86cf 0bfe 0001 44f8 e260
	0x0020:  0000 0000 16ab 0200 0000 0000 4242 4242
	0x0030:  4242 4242 4242 4242 4242 4242 4242 4242
	0x0040:  4242 4242 4242 4242 4242 4242 4242 4242
	0x0050:  4242 4242
```

You also can check DNS resolving via `tcpdump`:

```zsh
tcpdump -x "port 53"  
```


#### Complicated usage examples
```zsh
# available options: -vhsacDwVinqt, for more info ./ft_ping -h
./ft_ping -a -c 3 -D -i 2 -s 10 ya.ru
./ft_ping -a -w 10 ya.ru
```

```zsh
./ft_ping -h

Usage
  ./ft_ping [options] <destination>

Options:
  <destination>      dns name or ip address
  -a                 use audible ping
  -c <count>         stop after <count> replies
  -D                 print timestamps
  -h                 print help and exit
  -i <interval>      seconds between sending each packet
  -n                 no dns name resolution
  -q                 quiet output
  -s <size>          use <size> as number of data bytes to be sent
  -t <ttl>           define time to live
  -v                 verbose output
  -V                 print version and exit
  -w <deadline>      reply wait <deadline> in seconds
```

### Useful links for further exploration
* http://www.netpatch.ru/windows-files/pingscan/raw_ping.c.html

* http://www.n-cg.net/hec.htm

* https://habr.com/ru/post/352300/

* https://masandilov.ru/network/guide_to_network_programming5

* https://beej.us/guide/bgnet/html/#getaddrinfoman

* https://perso.telecom-paristech.fr/drossi/paper/rossi17ipid.pdf