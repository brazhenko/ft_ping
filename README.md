# ft_ping

### Prepare environment

```bash
docker build -t remote -f Dockerfile.remote .
docker run -d --cap-add sys_ptrace -p127.0.0.1:2222:22 --name remote_env remote
ssh-keygen -f "$HOME/.ssh/known_hosts" -R "[localhost]:2222"
```


### Usage

```bash
./ft_ping google.com
```

Default message ping `echo`s is 84 bytes length.
We can track how does our ping do in other terminal:

```bash
tcpdump -x "ip[2:2] == 84"
```

```
19:46:14.379152 IP 9386ef1efe85 > lo-in-f101.1e100.net: ICMP echo request, id 3770, seq 1, length 64
	0x0000:  4500 0054 d0f9 4000 4001 3174 ac11 0002
	0x0010:  adc2 de65 0800 bccf 0eba 0001 8678 db60
	0x0020:  0000 0000 06c9 0500 0000 0000 1011 1213
	0x0030:  1415 1617 1819 1a1b 1c1d 1e1f 2021 2223
	0x0040:  2425 2627 2829 2a2b 2c2d 2e2f 3031 3233
	0x0050:  3435 3637
19:46:14.395831 IP lo-in-f101.1e100.net > 9386ef1efe85: ICMP echo reply, id 3770, seq 1, length 64
	0x0000:  4500 0054 7d22 0000 2501 e04b adc2 de65
	0x0010:  ac11 0002 0000 c4cf 0eba 0001 8678 db60
	0x0020:  0000 0000 06c9 0500 0000 0000 1011 1213
	0x0030:  1415 1617 1819 1a1b 1c1d 1e1f 2021 2223
	0x0040:  2425 2627 2829 2a2b 2c2d 2e2f 3031 3233
	0x0050:  3435 3637
```
#### Complicated usage examples
```bash
./ft_ping 
```

### Useful links for further exploration
* http://www.netpatch.ru/windows-files/pingscan/raw_ping.c.html

* http://www.n-cg.net/hec.htm

* https://habr.com/ru/post/352300/

* https://masandilov.ru/network/guide_to_network_programming5

* https://beej.us/guide/bgnet/html/#getaddrinfoman

* https://perso.telecom-paristech.fr/drossi/paper/rossi17ipid.pdf