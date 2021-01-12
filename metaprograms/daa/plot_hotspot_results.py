import json
import matplotlib.pyplot as plt

def plot_hotspot_results(times, threshold):

    fig = plt.figure()

    x_pos = list(range(len(times)-1))
    ts = [times[t] for t in times if t != 'main']
    # print(ts)
    labels = [t for t in times if t != 'main']
    # print(labels)
    plt.bar(x_pos, ts, align='center', alpha=0.5)
    ax = plt.gca()
    ax.axhline(y=threshold, color='red')
    plt.xticks(x_pos, labels, rotation='vertical')
    plt.ylim(0, 100)
    plt.tight_layout()
    plt.savefig('./hotspot-result/hotspots.png')
    # plt.savefig('test_hotspots.png')
# 
# plot_hotspot_results({'main': 100, 'a': 60, 'b':20}, 50)
