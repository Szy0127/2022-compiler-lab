def is_prime(n):
	for i in range(2,n//2+1):
		if n % i == 0:
			return False
	return True
count = 0
prime_list = []
for i in range(2,10000):
	if is_prime(i):
		count += 1
		if count % 100 == 0:
			prime_list.append(i)
print(count)
for i in range(len(prime_list)):
	print(prime_list[i])
