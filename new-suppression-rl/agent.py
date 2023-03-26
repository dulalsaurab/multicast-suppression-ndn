# Try using different reinforcement learning algorithms, such as Proximal Policy Optimization (PPO) or Deep Q-Networks (DQN), 
# and compare their performance to the Actor-Critic algorithm.

import tensorflow as tf
from tensorflow.keras.optimizers import Adam
import tensorflow_probability as tfp
from network import ActorCriticNetwork

class Agent:
  def __init__(self, alpha=0.0003, gamma=0.99, n_actions=4):
    self.gamma = gamma
    self.n_actions = n_actions
    self.action = None          #keep track of last action
    self.action_space = [0, 10] #random action selection
    self.actor_critic = ActorCriticNetwork(n_actions=n_actions)
    self.actor_critic.compile(optimizer=Adam(learning_rate=alpha))

  def choose_action(self, observation):
    state = tf.convert_to_tensor([observation])
    _, probs = self.actor_critic(state)     #feed through neural network
    
    # action_probabilities = tfp.distributions.Normal(loc=probs[0][0], scale=probs[0][1])
    print(probs)
    mean = probs[0][0]
    std_dev = scale=probs[0][1]

    action_probabilities = tfp.distributions.Normal(loc=mean, scale=std_dev)
    action = action_probabilities.sample()
    log_prob = action_probabilities.log_prob(action)
    
    # log_prob = tf.reduce_sum(tfp.distributions.Normal(mean, std_dev).log_prob(action))

    print("Log Probability, ====", log_prob)
    self.action = action
    return action.numpy()

  def save_models(self):
    print('... saving models ...')
    self.actor_critic.save_weights(self.actor_critic.checkpoint_file)

  def load_models(self):
    print('... loading models ...')
    self.actor_critic.load_weights(self.actor_critic.checkpoint_file)
    

    # learn the policy
  def learn(self, state, reward, state_, done):
    state = tf.convert_to_tensor([state], dtype=tf.float32)
    state_ = tf.convert_to_tensor([state_], dtype=tf.float32)
    reward = tf.convert_to_tensor([reward], dtype=tf.float32)
    done = tf.convert_to_tensor([int(done)], dtype=tf.float32)
      
    '''
    GradientTape is a TensorFlow tool that is used for automatic differentiation. 
    It allows you to record operations that are performed on tensors, and then 
    compute the gradients of any variables with respect to the recorded operations.

    GradientTape is often used to compute the gradients of the policy and value functions 
    with respect to the loss function, which allows the weights of the neural network 
    to be updated in a direction that improves the performance of the agent.
    '''
    with tf.GradientTape() as tape:
      # Compute predicted value and action probabilities
      value, probs = self.actor_critic(state)
      value_, _ = self.actor_critic(state_)

      # Compute advantages and TD error
      td_error = reward + self.gamma * value_ * (1 - done) - value
      advantage = td_error

      # Compute actor and critic losses
      dist = tfp.distributions.Normal(loc=probs[0][0], scale=probs[0][1])
      log_prob = dist.log_prob(self.action)
      actor_loss = -log_prob * tf.stop_gradient(advantage)
      critic_loss = tf.square(td_error)

      # Compute total loss
      loss = actor_loss + critic_loss

    # Compute gradients and update weights
    gradients = tape.gradient(loss, self.actor_critic.trainable_variables)
    self.actor_critic.optimizer.apply_gradients(zip(gradients, self.actor_critic.trainable_variables))


