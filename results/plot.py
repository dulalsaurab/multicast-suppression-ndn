import numpy as np
import matplotlib.pyplot as plt
import sys
import pandas as pd

root_dir = sys.argv[1]

# interest statistics
df = []
for i in [2,3,4]:
    df.append(pd.read_csv(root_dir+"/"+str(i)+".interest", sep='[ ,]',))

fig, axes = plt.subplots(nrows=3, ncols=1,  figsize=(12, 8))
plt.subplots_adjust(left=None, bottom=None, right=None, top=None, wspace=0.1, hspace=0.5)
fig.suptitle('suppression time vs interest packet sent time')

ax1 =axes[0]
ax2 = axes[1]
ax3 = axes[2]

ax1.set_title('at C1', fontsize='small')
ax2.set_title('at C2',  fontsize='small')
ax3.set_title('at C3', fontsize='small')

for i, x in enumerate(axes):
    node = 'fig {}, at c{}'.format(i, i)
    x.set_title(node,  fontsize='small')
    x.set_xlabel('----> time', fontsize = 'small')
    x.set_xticklabels([100, 200, 300, 400, 500, 600])
    x.set_ylabel('suppression time (ms)', fontsize = 'small')
    df[i].plot(ax=x, kind='line', y='st', color='b')
    x.legend(loc="upper left")

    x2 =x.twinx()
    x2.set_ylabel('ewma', fontsize='small')

    # x.set_xticklabels([100, 200, 300, 400, 500, 600]) 
    df_t = df[i]['ema'].apply(lambda x: x)
    df_t.plot(ax=x2, kind='line', y='ewma', color='g', label="ewma")

    # df_t1 = df[i]['dup'].apply(lambda x: x*50)
    # df_t1.plot(ax=x, kind='line', linestyle='dashed', y='ema', color='g')
    # x2.legend([st])
    x2.legend()
plt.savefig('after-fix.png', bbox_inches='tight')
