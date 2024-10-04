

def random():
    seed:int = 1;  # You can change this for different sequences
    a:int = 527657655;  # Multiplier
    c:int = 18903;  # Increment
    m:int = 2147483647;  
    ticket = 465    # len of number of tickets

    #Function to generate a pseudo-random number
    def rand_lcg():
        nonlocal seed, ticket
        seed = (a * seed + c) % m 
        while ticket > 0:
            ticket -= 1
        else:
            ticket = 465
        return seed % ticket


    for i in range(100):
        print(rand_lcg())

random() 