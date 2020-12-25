# Regress speed as function of power from Peloton ride CSV dumps
# Part of the PeloMon project: https://github.com/ihaque/pelomon
#
# Copyright 2020 Imran S Haque (imran@ihaque.org)
# Licensed under the CC-NY-NC 4.0 license
# (https://creativecommons.org/licenses/by-nc/4.0/).

from collections import defaultdict
import sys

import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from scipy.optimize import minimize_scalar
from sklearn.linear_model import LinearRegression
from sklearn.preprocessing import PolynomialFeatures


def polynomial_fit(all_data):
    xs = np.sqrt(all_data['power'].values).reshape(-1,1)
    ys = all_data['speed'].values
    poly = PolynomialFeatures(3)
    xs_poly = poly.fit_transform(xs)
    # fit_intercept False because it's already included in the PolynomialFeatures
    clf = LinearRegression(fit_intercept=False)
    clf.fit(xs_poly, ys)
    yhat = clf.predict(xs_poly)
    error = (yhat - ys)
    mad = np.median(np.abs(error))
    maxerr = np.abs(error).max()
    rmse = np.sqrt((error ** 2).mean())
    return {'xs': xs,
            'featurizer': poly,
            'model': clf,
            'yhat': yhat,
            'ys': ys,
            'error': error,
            'rmse': rmse,
            'mad': mad,
            'maxerr': maxerr}


def two_piece_polynomial_fit(all_data, metric='rmse'):
    def split_fit(power_threshold):
        lower = all_data.query('power < @power_threshold')
        upper = all_data.query('power >= @power_threshold')
        fits = {
            'lower' : polynomial_fit(lower) if len(lower) else defaultdict(int),
            'upper' : polynomial_fit(upper) if len(upper) else defaultdict(int)
        }
        if metric == 'maxerr':
            error = max(fits['lower'][metric], fits['upper'][metric])
        elif metric == 'rmse':
            error = ((fits['lower'][metric] ** 2) * len(lower) +
                     (fits['upper'][metric] ** 2) * len(upper)) / (len(lower) +
                                                                   len(upper))
            error = np.sqrt(error)
        return error, fits

    unique_powers = sorted(set(all_data['power']))
    best_err = np.inf
    best_thresh = np.nan
    best_fits = None
    for power in unique_powers:
        if power > 100:
            break
        err, fits = split_fit(power)
        if err < best_err:
            best_err = err
            best_thresh = power
            best_fits = fits
            print('Split at %.2f, error %f' % (power, err))
    # Combine vectors from split fits
    best = {
        x: np.concatenate((best_fits['lower'][x], best_fits['upper'][x]))
        for x in ('xs', 'yhat', 'ys', 'error')
    }
    best['featurizer'] = best_fits['lower']['featurizer']
    best['model_0'] = best_fits['lower']['model']
    best['model_1'] = best_fits['upper']['model']
    best['mad'] = np.median(np.abs(best['error']))
    best['maxerr'] = np.abs(best['error']).max()
    best['rmse'] = np.sqrt((best['error'] ** 2).mean())
    best['thresh'] = best_thresh
    return best_thresh, best


def make_plots(rider1, rider2):
    fig = plt.figure()
    plt.subplot(121)
    for i, res in enumerate((29.0, 33.0, 43.0, 48.0, 65.0)):
        color = 'bgckr'[i]
        data = rider1[rider1['resistance'] == res].sort_values('cadence')
        plt.scatter(data['cadence'], data['speed'], color=color,
                    label='%d' % res)
    plt.legend(title='Resistance')
    plt.xlabel('Cadence (rpm)')
    plt.ylabel('Speed (mph)')

    plt.subplot(122)
    rider1 = rider1.sort_values('power')
    rider2 = rider2.sort_values('power')
    plt.scatter(np.sqrt(rider1['power']), rider1['speed'],
                color='b', alpha=0.5, label='Rider 1')
    plt.scatter(np.sqrt(rider2['power']), rider2['speed'],
                color='r', alpha=0.3, label='Rider 2')
    plt.xlabel('$\\sqrt{power}$ (W$^{1/2}$)')
    plt.ylabel('Speed (mph)')
    plt.legend()
    return fig


if __name__ == '__main__':
    rider1 = pd.read_csv('rider1.csv')
    rider2 = pd.read_csv('rider2.csv')
    all_data = (pd.concat([rider1, rider2])
                  .drop_duplicates(subset=['speed', 'power']))

    mpl.rc('lines', linewidth=2)
    mpl.rc('axes', titlesize='x-large', labelsize='large')
    mpl.rc('legend', fontsize='large')

    fig = make_plots(rider1, rider2)
    fig.set_figwidth(8)
    fig.set_figheight(4)
    fig.tight_layout()
    fig.savefig('speed-functions.png', dpi=200, bbox_inches='tight')

    # Fit a polynomial model on powers of sqrt(power)
    fig = plt.figure()
    mpl.rc('axes', labelsize=24)
    mpl.rc('xtick', labelsize=18)
    mpl.rc('ytick', labelsize=18)
    onefit = polynomial_fit(all_data)
    thresh, twofit = two_piece_polynomial_fit(all_data)

    onefit_desc = ('RMSE %.3f, MAD %.3f, max-error %.3f\n'
                   '%.3f + %.3f $p^{1/2}$ + %.3f $p$ + %.3f $p^{3/2}$' %
                   ((onefit['rmse'], onefit['mad'], onefit['maxerr']) +
                    tuple(onefit['model'].coef_)))
    twofit_desc = ('RMSE %.3f, MAD %.3f, max-error %.3f\n'
                   '%.3f + %.3f $p^{1/2}$ + %.3f $p$ + %.3f $p^{3/2}$ for p < %d\n'
                   '%.3f + %.3f $p^{1/2}$ + %.3f $p$ + %.3f $p^{3/2}$ for p $\\geq$ %d' %
                   ((twofit['rmse'], twofit['mad'], twofit['maxerr']) +
                    tuple(twofit['model_0'].coef_) +
                    (twofit['thresh'],) + 
                    tuple(twofit['model_1'].coef_) +
                    (twofit['thresh'],)))
    # Plot curve fits
    ax = fig.add_subplot(1,2,1)
    x_1, y_1, yh_1 = (np.array(_) for _ in 
                      zip(*sorted(zip(onefit['xs'],
                                      onefit['ys'],
                                      onefit['yhat']))))
    x_2, y_2, yh_2 = (np.array(_) for _ in 
                      zip(*sorted(zip(twofit['xs'],
                                      twofit['ys'],
                                      twofit['yhat']))))
    ax.scatter(x_1, y_1, color='k',
               alpha=0.1)
    ax.plot(x_1, yh_1, color='b',
            label='One component:\n'+ onefit_desc)
    ax.plot(x_2, yh_2, color='r',
            label='Two component:\n'+twofit_desc)
    ax.legend(loc='lower right')
    ax.set_xlabel('$\\sqrt{power}$ (W$^{1/2}$)')
    ax.set_ylabel('Speed (mph)')
           
    # Plot error
    ax = fig.add_subplot(1,2,2)
    xsort, esort = zip(*sorted(list(zip(onefit['xs']**2, onefit['error']))))
    plt.plot(xsort, esort, 'b', label='One component fit',
             alpha=0.5)
    xsort, esort = zip(*sorted(list(zip(twofit['xs']**2, twofit['error']))))
    plt.plot(xsort, esort, 'r', label='Two component fit',
             alpha=0.5)
    plt.axhline(0, color='k')
    plt.xlabel('power (W)')
    plt.ylabel('error (mph)')
    plt.legend()
    fig.set_figwidth(16)
    fig.set_figheight(8)
    fig.tight_layout()
    fig.savefig('speed-regressions.png', dpi=200, bbox_inches='tight')
    plt.show()
