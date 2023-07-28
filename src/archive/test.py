import gym
env = gym.make('CartPole-v0')
env.reset()
print (env.action_space)
for _ in range(1000):
    #env.render()
    env.step(env.action_space.sample()) # take a random action
    #print(env.action_space)
env.close()
